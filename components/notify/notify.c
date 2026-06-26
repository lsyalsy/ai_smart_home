#include "notify.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "NOTIFY";

/* Server酱 API 地址模板 */
#define SERVER_CHAN_URL_FMT "https://sctapi.ftqq.com/%s.send"

static char s_send_key[64] = {0};
static bool s_initialized = false;

bool notify_init(const char *server_chan_key)
{
    s_initialized = false;
    s_send_key[0] = '\0';

    if (server_chan_key == NULL || server_chan_key[0] == '\0') {
        ESP_LOGW(TAG, "no ServerChan key configured, notify disabled");
        return false;
    }

    strncpy(s_send_key, server_chan_key, sizeof(s_send_key) - 1);
    s_send_key[sizeof(s_send_key) - 1] = '\0';
    s_initialized = true;

    ESP_LOGI(TAG, "notify init ok");
    return true;
}

bool notify_send(const char *title, const char *content)
{
    if (!s_initialized || s_send_key[0] == '\0') {
        ESP_LOGW(TAG, "notify skipped: not initialized");
        return false;
    }

    if (title == NULL) {
        title = "";
    }
    if (content == NULL) {
        content = "";
    }

    char url[128];
    snprintf(url, sizeof(url), SERVER_CHAN_URL_FMT, s_send_key);

    char post_data[384];
    snprintf(post_data, sizeof(post_data), "title=%s&desp=%s", title, content);

    esp_http_client_config_t cfg = {
        .url                = url,
        .method             = HTTP_METHOD_POST,
        .timeout_ms         = 15000,
        .skip_cert_common_name_check = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "http client init failed");
        return false;
    }

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "notify send failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "notify send ok, http status=%d", status_code);
    return (status_code == 200);
}
