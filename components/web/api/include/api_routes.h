#ifndef __API_ROUTES_H
#define __API_ROUTES_H

#include "esp_http_server.h"

/**
 * @brief GET / - 返回控制面板首页 HTML
 */
esp_err_t api_handle_root(httpd_req_t *req);

/**
 * @brief GET /api/status - 返回传感器与执行器状态 JSON
 */
esp_err_t api_handle_status(httpd_req_t *req);

/**
 * @brief GET /api/led?on=1|0 - 控制 LED 开关
 */
esp_err_t api_handle_led(httpd_req_t *req);

/**
 * @brief GET /api/fan?level=0~20 - 设置风扇档位
 */
esp_err_t api_handle_fan(httpd_req_t *req);

/**
 * @brief GET /api/relay?ch=0|1&on=1|0 - 控制继电器通道
 */
esp_err_t api_handle_relay(httpd_req_t *req);

/**
 * @brief GET /api/buzzer?on=1|0 - 控制蜂鸣器
 */
esp_err_t api_handle_buzzer(httpd_req_t *req);

#endif /* __API_ROUTES_H */
