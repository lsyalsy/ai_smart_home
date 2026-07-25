/*
 * ESP32 大模型 HTTPS 交互模块 - 头文件
 * --------------------------------------------------
 * 功能：
 *   1. 通过 esp_http_client + esp-tls 建立 HTTPS 连接
 *   2. 使用 crt_bundle 自动加载根证书（无需手动嵌入）
 *   3. 支持 OpenAI 兼容 API（豆包、智谱、阿里云、OpenAI 等）
 *   4. 非流式响应模式（适合资源受限设备）
 *   5. 简洁的消息发送接口
 *
 * 依赖：ESP-IDF esp_http_client、esp-tls、cJSON 组件
 *
 * 使用示例：
 *   chat_init(API_KEY, BASE_URL, MODEL_NAME);
 *   char response[2048] = {0};
 *   chat_send_message("你好", response, sizeof(response));
 *   ESP_LOGI(TAG, "响应: %s", response);
 */

#ifndef __CHAT_H
#define __CHAT_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 错误码定义
 */
typedef enum {
    CHAT_OK = 0,           // 成功
    CHAT_ERR_NOT_INIT,     // 未初始化
    CHAT_ERR_WIFI_OFFLINE, // WiFi 未连接
    CHAT_ERR_CONNECT_FAIL, // 连接失败
    CHAT_ERR_REQUEST_FAIL, // 请求发送失败
    CHAT_ERR_HTTP_ERROR,   // HTTP 错误响应
    CHAT_ERR_JSON_PARSE,   // JSON 解析失败
    CHAT_ERR_TIMEOUT,      // 请求超时
    CHAT_ERR_MEMORY,       // 内存不足
} chat_error_t;

/**
 * @brief 初始化大模型客户端
 *
 * @param api_key    API 密钥
 * @param base_url   基础 URL（如 "https://api.doubao.com/v1"）
 * @param model_name 模型名称（如 "Doubao-lite"）
 * @param timeout_ms 请求超时时间（毫秒），默认 15000
 */
void chat_init(const char *api_key, const char *base_url, const char *model_name,
               unsigned int timeout_ms);

/**
 * @brief 发送消息给大模型
 *
 * @param message    用户消息
 * @param response   响应缓冲区（调用者分配）
 * @param resp_size  响应缓冲区大小
 * @return           chat_error_t 错误码
 */
chat_error_t chat_send_message(const char *message, char *response, size_t resp_size);

/**
 * @brief 获取最后一次错误码
 */
chat_error_t chat_get_last_error(void);

/**
 * @brief 获取错误描述字符串
 */
const char *chat_get_error_string(chat_error_t err);

/**
 * @brief 获取最后一次 HTTP 状态码
 */
int chat_get_last_http_code(void);

#ifdef __cplusplus
}
#endif

#endif /* __CHAT_H */
