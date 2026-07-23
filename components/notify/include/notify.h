#ifndef __NOTIFY_H
#define __NOTIFY_H

#include <stdbool.h>

/**
 * @brief 初始化微信推送（PushPlus）。
 *
 * @param pushplus_token PushPlus token，为空则禁用推送。
 * @return true 初始化成功，false 无有效 token
 */
bool notify_init(const char *pushplus_token);

/**
 * @brief 发送一条微信推送消息。
 *
 * @param title   消息标题
 * @param content 消息正文
 * @return true 发送成功（HTTP 200），false 失败或被禁用
 */
bool notify_send(const char *title, const char *content);

#endif /* __NOTIFY_H */
