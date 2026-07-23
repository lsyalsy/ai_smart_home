#ifndef __EC11_H
#define __EC11_H

#include <stdint.h>
#include <stdbool.h>

/* 项目设计：EC11 旋转编码器
 * CLK(A相) -> GPIO38
 * DT (B相) -> GPIO39
 * SW (按键) -> GPIO40
 * VCC -> 3.3V
 * GND -> GND
 */
#define EC11_CLK_GPIO  39
#define EC11_DT_GPIO   40
#define EC11_SW_GPIO   13

/**
 * @brief 初始化 EC11 旋转编码器 GPIO 与中断。
 */
void ec11_init(void);

/**
 * @brief 读取编码器旋转增量（顺时针为正，逆时针为负），读取后清零。
 * @return int16_t 累计脉冲数。
 */
int16_t ec11_read_delta(void);

/**
 * @brief 读取按键是否被按下，读取后清零。
 * @return true 按键被按下一次。
 */
bool ec11_button_pressed(void);

#endif /* __EC11_H */
