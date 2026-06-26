#ifndef __RELAY_H
#define __RELAY_H

#include <stdbool.h>

/* 项目设计：加湿器继电器接 GPIO17，卫生间换气扇继电器接 GPIO18 */
#define RELAY_HUMIDIFIER_GPIO   17
#define RELAY_BATHROOM_GPIO     18

typedef enum {
    RELAY_CHANNEL_HUMIDIFIER = 0,
    RELAY_CHANNEL_BATHROOM,
    RELAY_CHANNEL_MAX
} relay_channel_t;

/**
 * @brief 初始化所有继电器 GPIO。
 */
void relay_init(void);

/**
 * @brief 设置指定通道继电器状态。
 */
void relay_set(relay_channel_t ch, bool on);

/**
 * @brief 读取指定通道继电器状态。
 */
bool relay_get(relay_channel_t ch);

/**
 * @brief 切换指定通道继电器状态。
 */
void relay_toggle(relay_channel_t ch);

/* 以下兼容旧接口，默认操作加湿器通道 */
void relay_on(void);
void relay_off(void);
void relay_humidifier_toggle(void);
bool relay_state(void);

#endif /* __RELAY_H */
