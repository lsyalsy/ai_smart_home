#include "http_server.h"
#include "api_routes.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "WEB";

/* WiFi 配置 - 实际使用时应通过 menuconfig 或此处修改 */
#define WIFI_SSID       "AI-Smart-Home"
#define WIFI_PASSWORD   "12345678"
#define AP_SSID         "ESP32_Setup"
#define AP_PASSWORD     "12345678"

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_MAX_RETRY      5

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;
static httpd_handle_t s_server = NULL;
static bool s_wifi_init_done = false;

/* ---------- WiFi 事件处理 ---------- */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGI(TAG, "WiFi retry %d/%d", s_retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGW(TAG, "WiFi STA failed, switching to AP mode");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ---------- WiFi STA 模式连接 ---------- */
static bool wifi_connect_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        return true;
    }
    ESP_LOGW(TAG, "WiFi STA connection failed");
    return false;
}

/* ---------- WiFi AP 热点模式 ---------- */
static void wifi_start_ap(void)
{
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    ESP_LOGI(TAG, "AP mode started: SSID=%s  IP: " IPSTR, AP_SSID, IP2STR(&ip_info.ip));
}

/* ---------- 初始化 WiFi（NVS + NetIF + 事件循环） ---------- */
static esp_err_t wifi_init(void)
{
    if (s_wifi_init_done) return ESP_OK;

    /* NVS 初始化（WiFi 需要） */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 网络接口 + 事件循环 */
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    esp_wifi_set_mode(WIFI_MODE_STA);

    s_wifi_init_done = true;
    return ESP_OK;
}

/* ---------- HTTP 服务器启动 ---------- */
bool http_server_start(void)
{
    if (s_server != NULL) {
        ESP_LOGW(TAG, "HTTP server already running");
        return true;
    }

    /* 初始化 WiFi */
    wifi_init();

    /* 尝试 STA 模式连接，失败则切 AP 模式 */
    if (!wifi_connect_sta()) {
        wifi_start_ap();
    }

    /* 配置并启动 HTTP 服务器 */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
        return false;
    }

    /* 注册路由 */
    httpd_uri_t uri_root = {
        .uri = "/", .method = HTTP_GET, .handler = api_handle_root,
    };
    httpd_register_uri_handler(s_server, &uri_root);

    httpd_uri_t uri_status = {
        .uri = "/api/status", .method = HTTP_GET, .handler = api_handle_status,
    };
    httpd_register_uri_handler(s_server, &uri_status);

    httpd_uri_t uri_led = {
        .uri = "/api/led", .method = HTTP_GET, .handler = api_handle_led,
    };
    httpd_register_uri_handler(s_server, &uri_led);

    httpd_uri_t uri_fan = {
        .uri = "/api/fan", .method = HTTP_GET, .handler = api_handle_fan,
    };
    httpd_register_uri_handler(s_server, &uri_fan);

    httpd_uri_t uri_relay = {
        .uri = "/api/relay", .method = HTTP_GET, .handler = api_handle_relay,
    };
    httpd_register_uri_handler(s_server, &uri_relay);

    httpd_uri_t uri_buzzer = {
        .uri = "/api/buzzer", .method = HTTP_GET, .handler = api_handle_buzzer,
    };
    httpd_register_uri_handler(s_server, &uri_buzzer);

    ESP_LOGI(TAG, "HTTP server started on port 80");
    return true;
}

/* ---------- HTTP 服务器停止 ---------- */
void http_server_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}
