#ifndef __HTTP_SERVER_H
#define __HTTP_SERVER_H

#include <stdbool.h>

/**
 * @brief 启动 HTTP Web 服务器（含 WiFi STA/AP 连接）。
 *
 * 服务器启动后提供：
 *   - 首页控制面板（/）
 *   - 传感器数据 JSON（/api/status）
 *   - LED 控制（/api/led?on=1|0）
 *   - 风扇控制（/api/fan?level=0~20）
 *   - 继电器控制（/api/relay?ch=0|1&on=1|0）
 *
 * WiFi 连接失败时自动切换为 AP 热点模式。
 *
 * @return true 启动成功，false 启动失败
 */
bool http_server_start(void);

/**
 * @brief 停止 HTTP Web 服务器。
 */
void http_server_stop(void);

#endif /* __HTTP_SERVER_H */
