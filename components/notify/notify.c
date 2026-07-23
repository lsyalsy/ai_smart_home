#include "notify.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "NOTIFY";

/* PushPlus API 地址 */
#define PUSHPLUS_URL  "http://www.pushplus.plus/send"

static char s_token[64] = {0};
static bool s_initialized = false;

bool notify_init(const char *pushplus_token)
{
    s_initialized = false;
    s_token[0] = '\0';

    if (pushplus_token == NULL || pushplus_token[0] == '\0') {
        ESP_LOGW(TAG, "no PushPlus token configured, notify disabled");
        return false;
    }

    strncpy(s_token, pushplus_token, sizeof(s_token) - 1);
    s_token[sizeof(s_token) - 1] = '\0';
    s_initialized = true;

    ESP_LOGI(TAG, "PushPlus notify init ok");
    return true;
}

bool notify_send(const char *title, const char *content)
{
    if (!s_initialized || s_token[0] == '\0') {
        ESP_LOGW(TAG, "notify skipped: not initialized");
        return false;
    }

    if (title == NULL) {
        title = "";
    }
    if (content == NULL) {
        content = "";
    }

    /* 构建 PushPlus JSON 请求体 */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "token", s_token);
    cJSON_AddStringToObject(root, "title", title);
    cJSON_AddStringToObject(root, "content", content);
    cJSON_AddStringToObject(root, "template", "txt");

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json_str) {
        ESP_LOGE(TAG, "json build failed");
        return false;
    }

    esp_http_client_config_t cfg = {
        .url                = PUSHPLUS_URL,
        .method             = HTTP_METHOD_POST,
        .timeout_ms         = 15000,
        .skip_cert_common_name_check = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "http client init failed");
        cJSON_free(json_str);
        return false;
    }

    esp_http_client_set_post_field(client, json_str, strlen(json_str));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    cJSON_free(json_str);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "notify send failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "notify send ok, http status=%d", status_code);
    return (status_code == 200);
}
