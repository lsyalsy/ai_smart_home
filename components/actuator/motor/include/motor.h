#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>
#include <stdbool.h>

/* 项目设计：风扇电机接 GPIO16，使用 LEDC PWM 调速 */
#define MOTOR_GPIO 16

/* 20 档调速，每档 5% 占空比 */
#define MOTOR_SPEED_MAX_LEVEL   20
#define MOTOR_SPEED_STEP_PERCENT 5

/**
 * @brief 初始化电机 PWM（LEDC）。
 */
void motor_init(void);

/**
 * @brief 反初始化电机 PWM。
 */
void motor_deinit(void);

/**
 * @brief 设置风扇转速档位（0 ~ MOTOR_SPEED_MAX_LEVEL）。
 * @param level 0 为关闭，20 为全速。
 */
void motor_set_speed(uint8_t level);

/**
 * @brief 获取当前风扇转速档位。
 */
uint8_t motor_get_speed(void);

/**
 * @brief 打开电机（恢复到上次档位；若上次为 0 则全速）。
 */
void motor_on(void);

/**
 * @brief 关闭电机（档位设为 0）。
 */
void motor_off(void);

/**
 * @brief 切换电机开关状态。
 */
void motor_toggle(void);

/**
 * @brief 返回电机是否处于开启状态（档位 > 0）。
 */
bool motor_state(void);

#endif /* __MOTOR_H */
