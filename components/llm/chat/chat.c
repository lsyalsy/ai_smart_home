/*
 * ESP32 大模型 HTTPS 交互模块 - 实现文件
 */

#include "chat.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "string.h"

static const char *TAG = "CHAT";

/* 配置参数 */
static const char *s_api_key = NULL;
static const char *s_base_url = NULL;
static const char *s_model_name = NULL;
static unsigned int s_timeout_ms = 15000;
static chat_error_t s_last_error = CHAT_ERR_NOT_INIT;
static int s_last_http_code = 0;

/* ========== 错误处理 ========== */
chat_error_t chat_get_last_error(void)
{
    return s_last_error;
}

int chat_get_last_http_code(void)
{
    return s_last_http_code;
}

const char *chat_get_error_string(chat_error_t err)
{
    switch (err) {
        case CHAT_OK: return "成功";
        case CHAT_ERR_NOT_INIT: return "未初始化";
        case CHAT_ERR_WIFI_OFFLINE: return "WiFi 未连接";
        case CHAT_ERR_CONNECT_FAIL: return "连接失败";
        case CHAT_ERR_REQUEST_FAIL: return "请求发送失败";
        case CHAT_ERR_HTTP_ERROR: return "HTTP 错误响应";
        case CHAT_ERR_JSON_PARSE: return "JSON 解析失败";
        case CHAT_ERR_TIMEOUT: return "请求超时";
        case CHAT_ERR_MEMORY: return "内存不足";
        default: return "未知错误";
    }
}

static void chat_set_error(chat_error_t err)
{
    s_last_error = err;
    ESP_LOGI(TAG, "错误: %s", chat_get_error_string(err));
}

/* ========== JSON 字符串转义 ========== */
static int json_escape_string(char *dest, const char *src, size_t dest_size)
{
    size_t i = 0, j = 0;
    while (src[i] != '\0' && j < dest_size - 1) {
        switch (src[i]) {
            case '\\':
                if (j + 1 < dest_size) dest[j++] = '\\';
                if (j < dest_size) dest[j++] = '\\';
                break;
            case '\"':
                if (j + 1 < dest_size) dest[j++] = '\\';
                if (j < dest_size) dest[j++] = '\"';
                break;
            case '\n':
                if (j + 1 < dest_size) dest[j++] = '\\';
                if (j < dest_size) dest[j++] = 'n';
                break;
            case '\r':
                if (j + 1 < dest_size) dest[j++] = '\\';
                if (j < dest_size) dest[j++] = 'r';
                break;
            case '\t':
                if (j + 1 < dest_size) dest[j++] = '\\';
                if (j < dest_size) dest[j++] = 't';
                break;
            default:
                if (j < dest_size) dest[j++] = src[i];
                break;
        }
        i++;
    }
    dest[j] = '\0';
    return j;
}

/* ========== 构建请求 URL ========== */
static int chat_build_url(char *url, size_t url_size, const char *base_url)
{
    const char *path = "/chat/completions";
    
    /* 检查 base_url 是否已经包含完整路径 */
    if (strstr(base_url, "chat/completions")) {
        return snprintf(url, url_size, "%s", base_url);
    }
    
    /* 拼接 base_url + path */
    const char *suffix = (base_url[strlen(base_url)-1] == '/') ? "" : "/";
    return snprintf(url, url_size, "%s%s%s", base_url, suffix, path);
}

/* ========== 构建请求体 ========== */
static int chat_build_request_body(char *body, size_t body_size, const char *message, const char *model_name)
{
    char escaped[512];
    json_escape_string(escaped, message, sizeof(escaped));
    
    return snprintf(body, body_size, 
             "{\"model\":\"%s\",\"stream\":false,\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}]}",
             model_name, escaped);
}

/* ========== 初始化 ========== */
void chat_init(const char *api_key, const char *base_url, const char *model_name,
               unsigned int timeout_ms)
{
    s_api_key = api_key;
    s_base_url = base_url;
    s_model_name = model_name;
    s_timeout_ms = (timeout_ms > 0) ? timeout_ms : 15000;
    s_last_error = CHAT_OK;
    
    ESP_LOGI(TAG, "大模型客户端初始化完成");
    ESP_LOGI(TAG, "模型: %s, Base URL: %s, 超时: %ums", model_name, base_url, s_timeout_ms);
}

/* ========== 发送消息核心函数 ========== */
chat_error_t chat_send_message(const char *message, char *response, size_t resp_size)
{
    /* 参数检查 */
    if (!s_api_key || !s_base_url || !s_model_name) {
        chat_set_error(CHAT_ERR_NOT_INIT);
        return CHAT_ERR_NOT_INIT;
    }
    
    if (!message || !response || resp_size < 64) {
        chat_set_error(CHAT_ERR_MEMORY);
        return CHAT_ERR_MEMORY;
    }
    
    /* WiFi 状态检查 */
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        chat_set_error(CHAT_ERR_WIFI_OFFLINE);
        return CHAT_ERR_WIFI_OFFLINE;
    }
    
    /* 构建 URL 和请求体（使用栈缓冲区，线程安全） */
    char url[256];
    char body[1024];
    chat_build_url(url, sizeof(url), s_base_url);
    chat_build_request_body(body, sizeof(body), message, s_model_name);
    
    ESP_LOGD(TAG, "请求 URL: %s", url);
    ESP_LOGD(TAG, "请求体: %s", body);
    
    /* 配置 HTTP 客户端 */
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = s_timeout_ms,
        .keep_alive_enable = false,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        chat_set_error(CHAT_ERR_MEMORY);
        return CHAT_ERR_MEMORY;
    }
    
    /* 设置请求头 */
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_api_key);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    /* 使用手动 API 流程：open -> write -> fetch_headers -> read -> close */
    ESP_LOGI(TAG, "正在发送请求...");
    
    /* 打开连接 */
    int body_len = strlen(body);
    esp_err_t err = esp_http_client_open(client, body_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP 打开连接失败: %s", esp_err_to_name(err));
        chat_set_error(CHAT_ERR_CONNECT_FAIL);
        esp_http_client_cleanup(client);
        return CHAT_ERR_CONNECT_FAIL;
    }
    
    /* 写入请求体 */
    int written = esp_http_client_write(client, body, body_len);
    if (written != body_len) {
        ESP_LOGE(TAG, "HTTP 写入请求体失败: %d/%d", written, body_len);
        chat_set_error(CHAT_ERR_REQUEST_FAIL);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return CHAT_ERR_REQUEST_FAIL;
    }
    
    /* 获取响应头 */
    int status_code = esp_http_client_fetch_headers(client);
    s_last_http_code = status_code;
    ESP_LOGI(TAG, "HTTP 状态码: %d", status_code);
    
    if (status_code != 200) {
        /* 读取错误响应内容（最多 500 字节） */
        char error_body[512] = {0};
        int read_len = esp_http_client_read(client, error_body, sizeof(error_body) - 1);
        if (read_len > 0) {
            ESP_LOGE(TAG, "错误响应: %s", error_body);
        }
        chat_set_error(CHAT_ERR_HTTP_ERROR);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return CHAT_ERR_HTTP_ERROR;
    }
    
    /* 读取响应体 */
    int content_len = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "响应长度: %d", content_len);
    
    /* 限制读取大小，避免缓冲区溢出 */
    int max_read = (content_len > 0 && content_len < (int)resp_size) ? content_len : (int)(resp_size - 1);
    int read_len = esp_http_client_read(client, response, max_read);
    
    if (read_len <= 0) {
        ESP_LOGE(TAG, "读取响应失败");
        chat_set_error(CHAT_ERR_REQUEST_FAIL);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return CHAT_ERR_REQUEST_FAIL;
    }
    
    response[read_len] = '\0';
    
    /* 关闭连接 */
    esp_http_client_close(client);
    
    /* 解析 JSON，提取 content */
    cJSON *root = cJSON_Parse(response);
    if (!root) {
        ESP_LOGE(TAG, "JSON 解析失败");
        chat_set_error(CHAT_ERR_JSON_PARSE);
        esp_http_client_cleanup(client);
        return CHAT_ERR_JSON_PARSE;
    }
    
    /* 提取 choices[0].message.content */
    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (!choices || !cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        ESP_LOGE(TAG, "找不到 choices 字段");
        cJSON_Delete(root);
        chat_set_error(CHAT_ERR_JSON_PARSE);
        esp_http_client_cleanup(client);
        return CHAT_ERR_JSON_PARSE;
    }
    
    cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
    cJSON *message_obj = cJSON_GetObjectItem(first_choice, "message");
    cJSON *content = cJSON_GetObjectItem(message_obj, "content");
    
    if (!content || !cJSON_IsString(content)) {
        ESP_LOGE(TAG, "找不到 content 字段");
        cJSON_Delete(root);
        chat_set_error(CHAT_ERR_JSON_PARSE);
        esp_http_client_cleanup(client);
        return CHAT_ERR_JSON_PARSE;
    }
    
    /* 复制提取到的内容到响应缓冲区 */
    size_t content_len_out = strlen(content->valuestring);
    if (content_len_out >= resp_size) {
        content_len_out = resp_size - 1;
    }
    memcpy(response, content->valuestring, content_len_out);
    response[content_len_out] = '\0';
    
    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    
    ESP_LOGI(TAG, "响应内容: %s", response);
    s_last_error = CHAT_OK;
    return CHAT_OK;
}
