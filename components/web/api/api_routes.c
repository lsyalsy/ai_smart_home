#include "api_routes.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"

#include "led.h"
#include "buzzer.h"
#include "motor.h"
#include "relay.h"
#include "dht22.h"
#include "bh1750.h"
#include "hcsr501.h"
#include "bathroom_pir.h"
#include "mq2.h"

/* 需要访问全局状态，通过 extern 声明 */
#include "ui.h"
extern system_state_t g_state;

static const char *TAG = "API";

/* ---------- 工具：解析 query 参数 ---------- */
static int query_get_int(const char *query, const char *key, int default_val)
{
    if (!query || !key) return default_val;
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "%s=", key);
    const char *p = strstr(query, pattern);
    if (!p) return default_val;
    p += strlen(pattern);
    return atoi(p);
}

/* ---------- GET / - 首页 HTML ---------- */
esp_err_t api_handle_root(httpd_req_t *req)
{
    extern const char index_html_start[] asm("_binary____static_index_html_start");
    extern const char index_html_end[]   asm("_binary____static_index_html_end");
    size_t len = index_html_end - index_html_start;

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, index_html_start, len);
    return ESP_OK;
}

/* ---------- GET /api/status - 状态 JSON ---------- */
esp_err_t api_handle_status(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    /* 传感器数据（直接读取，确保实时性） */
    float temp = 0, hum = 0;
    dht22_read(&temp, &hum);

    cJSON_AddNumberToObject(root, "temperature", temp);
    cJSON_AddNumberToObject(root, "humidity", hum);
    cJSON_AddNumberToObject(root, "light", bh1750_read_lux());
    cJSON_AddBoolToObject(root, "human_present", hcsr501_detected());
    cJSON_AddBoolToObject(root, "bathroom_pir", bathroom_pir_detected());
    cJSON_AddNumberToObject(root, "smoke_raw", mq2_read_raw());
    cJSON_AddBoolToObject(root, "smoke_alarm", g_state.smoke_alarm);
    cJSON_AddNumberToObject(root, "heart_rate", g_state.heart_rate);

    /* 执行器状态 */
    cJSON_AddBoolToObject(root, "led_on", led_state());
    cJSON_AddNumberToObject(root, "fan_speed", motor_get_speed());
    cJSON_AddBoolToObject(root, "humidifier_on", relay_get(RELAY_CHANNEL_HUMIDIFIER));
    cJSON_AddBoolToObject(root, "bathroom_fan_on", relay_get(RELAY_CHANNEL_BATHROOM));
    cJSON_AddBoolToObject(root, "buzzer_on", g_state.alarm_triggered);

    /* 系统信息 */
    cJSON_AddNumberToObject(root, "uptime", (double)(esp_timer_get_time() / 1000000));
    cJSON_AddNumberToObject(root, "free_heap", (double)esp_get_free_heap_size());

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json_str, strlen(json_str));

    cJSON_free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/* ---------- GET /api/led?on=1|0 ---------- */
esp_err_t api_handle_led(httpd_req_t *req)
{
    char query[64] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        int on = query_get_int(query, "on", -1);
        if (on == 1) {
            led_on();
        } else if (on == 0) {
            led_off();
        } else {
            led_toggle();
        }
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "led_on", led_state());
    const char *json_str = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    cJSON_free((void *)json_str);
    cJSON_Delete(resp);
    return ESP_OK;
}

/* ---------- GET /api/fan?level=0~20 ---------- */
esp_err_t api_handle_fan(httpd_req_t *req)
{
    char query[64] = {0};
    int level = -1;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        level = query_get_int(query, "level", -1);
    }

    if (level >= 0 && level <= MOTOR_SPEED_MAX_LEVEL) {
        motor_set_speed((uint8_t)level);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "fan_speed", motor_get_speed());
    const char *json_str = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    cJSON_free((void *)json_str);
    cJSON_Delete(resp);
    return ESP_OK;
}

/* ---------- GET /api/relay?ch=0|1&on=1|0 ---------- */
esp_err_t api_handle_relay(httpd_req_t *req)
{
    char query[64] = {0};
    int ch = 0, on = -1;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        ch = query_get_int(query, "ch", 0);
        on = query_get_int(query, "on", -1);
    }

    relay_channel_t channel = (ch == 0) ? RELAY_CHANNEL_HUMIDIFIER : RELAY_CHANNEL_BATHROOM;
    if (on == 1) {
        relay_set(channel, true);
    } else if (on == 0) {
        relay_set(channel, false);
    } else {
        relay_toggle(channel);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "ch", ch);
    cJSON_AddBoolToObject(resp, "on", relay_get(channel));
    const char *json_str = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    cJSON_free((void *)json_str);
    cJSON_Delete(resp);
    return ESP_OK;
}

/* ---------- GET /api/buzzer?on=1|0 ---------- */
esp_err_t api_handle_buzzer(httpd_req_t *req)
{
    char query[64] = {0};
    int on = -1;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        on = query_get_int(query, "on", -1);
    }

    if (on == 1) {
        buzzer_on();
    } else if (on == 0) {
        buzzer_off();
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "buzzer_on", g_state.alarm_triggered);
    const char *json_str = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    cJSON_free((void *)json_str);
    cJSON_Delete(resp);
    return ESP_OK;
}
