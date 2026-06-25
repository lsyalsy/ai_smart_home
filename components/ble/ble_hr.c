#include "ble_hr.h"

#include <string.h>
#include <stdint.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "os/os_mbuf.h"

static const char *TAG = "BLE_HR";

/* 心率服务 / 测量特征 / CCCD 的 16-bit UUID */
#define HEART_RATE_SERVICE_UUID     0x180D
#define HEART_RATE_MEASUREMENT_UUID 0x2A37
#define CLIENT_CHAR_CFG_UUID        0x2902

static SemaphoreHandle_t s_hr_mutex = NULL;
static uint8_t  s_heart_rate = 0;     /* 最近心率值 */
static bool     s_connected  = false; /* 是否已连接 */
static bool     s_connecting = false; /* 是否正在发起连接 */
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

/* GATT 发现过程中缓存的句柄 */
static uint16_t s_hr_svc_start = 0;
static uint16_t s_hr_svc_end   = 0;
static uint16_t s_hr_chr_val_handle = 0;
static uint16_t s_hr_cccd_handle    = 0;

/* 前向声明 */
static void ble_hr_scan(void);
static void ble_hr_start_discovery(uint16_t conn_handle);
static int ble_hr_gap_event(struct ble_gap_event *event, void *arg);
static int ble_hr_disc_svc_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              const struct ble_gatt_svc *service,
                              void *arg);
static int ble_hr_disc_chr_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              const struct ble_gatt_chr *chr,
                              void *arg);
static int ble_hr_disc_dsc_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              uint16_t chr_val_handle,
                              const struct ble_gatt_dsc *dsc,
                              void *arg);
static int ble_hr_subscribe_cb(uint16_t conn_handle,
                               const struct ble_gatt_error *error,
                               struct ble_gatt_attr *attr,
                               void *arg);

static void ble_hr_set_heart_rate(uint8_t hr)
{
    if (s_hr_mutex == NULL) {
        return;
    }
    xSemaphoreTake(s_hr_mutex, portMAX_DELAY);
    s_heart_rate = hr;
    xSemaphoreGive(s_hr_mutex);
}

uint8_t ble_hr_get_heart_rate(void)
{
    uint8_t hr = 0;
    if (s_hr_mutex == NULL) {
        return 0;
    }
    xSemaphoreTake(s_hr_mutex, portMAX_DELAY);
    hr = s_heart_rate;
    xSemaphoreGive(s_hr_mutex);
    return hr;
}

bool ble_hr_is_connected(void)
{
    return s_connected;
}

/* 开始扫描广播了心率服务的设备 */
static void ble_hr_scan(void)
{
    struct ble_gap_disc_params disc_params = {0};
    uint8_t own_addr_type;
    int rc;

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "error determining address type; rc=%d", rc);
        return;
    }

    disc_params.filter_duplicates = 1;
    disc_params.passive = 1;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      ble_hr_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "error initiating GAP discovery; rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "started scanning for HR devices");
    }
}

/* 连接到感兴趣的广播设备 */
static void ble_hr_connect_if_interesting(const struct ble_gap_disc_desc *disc)
{
    uint8_t own_addr_type;
    int rc;

    /* 只处理可连接的广播 */
    if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
        disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return;
    }

    /* 解析广播数据 */
    struct ble_hs_adv_fields fields;
    rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
    if (rc != 0) {
        return;
    }

    /* 查找心率服务 0x180D */
    bool has_hr_svc = false;
    for (int i = 0; i < fields.num_uuids16; i++) {
        if (ble_uuid_u16(&fields.uuids16[i].u) == HEART_RATE_SERVICE_UUID) {
            has_hr_svc = true;
            break;
        }
    }

    if (!has_hr_svc) {
        return;
    }

    /* 避免重复发起连接 */
    if (s_connected || s_connecting) {
        return;
    }

    ESP_LOGI(TAG, "found HR device, addr=%02x:%02x:%02x:%02x:%02x:%02x",
             disc->addr.val[5], disc->addr.val[4], disc->addr.val[3],
             disc->addr.val[2], disc->addr.val[1], disc->addr.val[0]);

    rc = ble_gap_disc_cancel();
    if (rc != 0) {
        ESP_LOGW(TAG, "failed to cancel scan; rc=%d", rc);
        return;
    }

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "error determining address type; rc=%d", rc);
        return;
    }

    s_connecting = true;
    rc = ble_gap_connect(own_addr_type, &disc->addr, 30000, NULL,
                         ble_hr_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "error connecting to device; rc=%d", rc);
        s_connecting = false;
        ble_hr_scan();
        return;
    }

    ESP_LOGI(TAG, "connecting...");
}

/* 订阅完成后的写回调 */
static int ble_hr_subscribe_cb(uint16_t conn_handle,
                               const struct ble_gatt_error *error,
                               struct ble_gatt_attr *attr,
                               void *arg)
{
    if (error->status == 0) {
        ESP_LOGI(TAG, "HR notifications subscribed, cccd_handle=%d", attr->handle);
    } else {
        ESP_LOGE(TAG, "subscribe write failed; status=%d", error->status);
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    return 0;
}

/* 描述符发现回调 */
static int ble_hr_disc_dsc_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              uint16_t chr_val_handle,
                              const struct ble_gatt_dsc *dsc,
                              void *arg)
{
    if (error->status == BLE_HS_EDONE) {
        if (s_hr_cccd_handle != 0) {
            uint8_t value[2] = {0x01, 0x00};
            int rc = ble_gattc_write_flat(conn_handle, s_hr_cccd_handle,
                                          value, sizeof(value),
                                          ble_hr_subscribe_cb, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "failed to write CCCD; rc=%d", rc);
                ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
        } else {
            ESP_LOGE(TAG, "CCCD not found");
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }

    if (error->status != 0) {
        ESP_LOGE(TAG, "descriptor discovery failed; status=%d", error->status);
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }

    if (dsc != NULL &&
        ble_uuid_cmp(&dsc->uuid.u, BLE_UUID16_DECLARE(CLIENT_CHAR_CFG_UUID)) == 0) {
        s_hr_cccd_handle = dsc->handle;
    }
    return 0;
}

/* 特征发现回调 */
static int ble_hr_disc_chr_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              const struct ble_gatt_chr *chr,
                              void *arg)
{
    if (error->status == BLE_HS_EDONE) {
        if (s_hr_chr_val_handle != 0) {
            ESP_LOGI(TAG, "HR measurement char found, val_handle=%d", s_hr_chr_val_handle);
            int rc = ble_gattc_disc_all_dscs(conn_handle, s_hr_chr_val_handle,
                                             s_hr_svc_end, ble_hr_disc_dsc_cb, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "failed to start descriptor discovery; rc=%d", rc);
                ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
        } else {
            ESP_LOGE(TAG, "HR measurement characteristic not found");
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }

    if (error->status != 0) {
        ESP_LOGE(TAG, "characteristic discovery failed; status=%d", error->status);
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }

    if (chr != NULL &&
        ble_uuid_cmp(&chr->uuid.u, BLE_UUID16_DECLARE(HEART_RATE_MEASUREMENT_UUID)) == 0) {
        s_hr_chr_val_handle = chr->val_handle;
    }
    return 0;
}

/* 服务发现回调 */
static int ble_hr_disc_svc_cb(uint16_t conn_handle,
                              const struct ble_gatt_error *error,
                              const struct ble_gatt_svc *service,
                              void *arg)
{
    if (error->status == BLE_HS_EDONE) {
        if (s_hr_svc_start != 0 && s_hr_svc_end != 0) {
            ESP_LOGI(TAG, "HR service found 0x%04x-0x%04x",
                     s_hr_svc_start, s_hr_svc_end);
            int rc = ble_gattc_disc_all_chrs(conn_handle, s_hr_svc_start,
                                             s_hr_svc_end, ble_hr_disc_chr_cb, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "failed to start characteristic discovery; rc=%d", rc);
                ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
        } else {
            ESP_LOGE(TAG, "HR service not found");
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }

    if (error->status != 0) {
        ESP_LOGE(TAG, "service discovery failed; status=%d", error->status);
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return 0;
    }

    if (service != NULL &&
        ble_uuid_cmp(&service->uuid.u, BLE_UUID16_DECLARE(HEART_RATE_SERVICE_UUID)) == 0) {
        s_hr_svc_start = service->start_handle;
        s_hr_svc_end   = service->end_handle;
    }
    return 0;
}

/* 发起 GATT 服务发现 */
static void ble_hr_start_discovery(uint16_t conn_handle)
{
    s_hr_svc_start = 0;
    s_hr_svc_end = 0;
    s_hr_chr_val_handle = 0;
    s_hr_cccd_handle = 0;

    int rc = ble_gattc_disc_all_svcs(conn_handle, ble_hr_disc_svc_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start service discovery; rc=%d", rc);
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
}

/* GAP 事件回调 */
static int ble_hr_gap_event(struct ble_gap_event *event, void *arg)
{
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        ble_hr_connect_if_interesting(&event->disc);
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        s_connecting = false;
        if (event->connect.status == 0) {
            s_connected = true;
            s_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "connected, conn_handle=%d", s_conn_handle);
            ble_hr_start_discovery(s_conn_handle);
        } else {
            ESP_LOGE(TAG, "connection failed; status=%d", event->connect.status);
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            s_connected = false;
            ble_hr_set_heart_rate(0);
            ble_hr_scan();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected; reason=%d", event->disconnect.reason);
        s_connected = false;
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        s_connecting = false;
        ble_hr_set_heart_rate(0);
        ble_hr_scan();
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX: {
        struct os_mbuf *om = event->notify_rx.om;
        uint16_t len = OS_MBUF_PKTLEN(om);
        if (len < 2) {
            return 0;
        }
        uint8_t data[4];
        uint16_t copy_len = len > sizeof(data) ? sizeof(data) : len;
        rc = os_mbuf_copydata(om, 0, copy_len, data);
        if (rc != 0) {
            return 0;
        }

        uint8_t flags = data[0];
        uint8_t hr;
        if (flags & 0x01) {
            /* uint16 心率值（小端） */
            if (len < 3) {
                return 0;
            }
            uint16_t hr16 = data[1] | (data[2] << 8);
            hr = (hr16 > 255) ? 255 : (uint8_t)hr16;
        } else {
            hr = data[1];
        }

        ble_hr_set_heart_rate(hr);
        ESP_LOGI(TAG, "HR %s received: %d bpm",
                 event->notify_rx.indication ? "indication" : "notification", hr);
        return 0;
    }

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU updated; conn_handle=%d mtu=%d",
                 event->mtu.conn_handle, event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

static void ble_hr_on_reset(int reason)
{
    ESP_LOGE(TAG, "resetting state; reason=%d", reason);
    s_connected = false;
    s_connecting = false;
    s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    ble_hr_set_heart_rate(0);
}

static void ble_hr_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to ensure identity address; rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "NimBLE synced");
    ble_hr_scan();
}

static void ble_hr_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_hr_init(void)
{
    ESP_LOGI(TAG, "initializing BLE HR monitor");

    s_hr_mutex = xSemaphoreCreateMutex();
    configASSERT(s_hr_mutex != NULL);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to init nimble; ret=%d", ret);
        return;
    }

    ble_hs_cfg.reset_cb = ble_hr_on_reset;
    ble_hs_cfg.sync_cb  = ble_hr_on_sync;

    nimble_port_freertos_init(ble_hr_host_task);
}
