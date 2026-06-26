#ifndef __BATHROOM_PIR_H
#define __BATHROOM_PIR_H

#include <stdbool.h>

/* 项目设计：卫生间人体红外感应接 GPIO5 */
#define BATHROOM_PIR_GPIO 5

/**
 * @brief 初始化卫生间 PIR 人体红外传感器。
 */
void bathroom_pir_init(void);

/**
 * @brief 读取当前是否检测到卫生间有人。
 */
bool bathroom_pir_detected(void);

#endif /* __BATHROOM_PIR_H */
