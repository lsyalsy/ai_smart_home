#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 面包板方案：TB6612FNG 双通道电机驱动
 *
 * A 通道（风扇1）:
 *   AIN1 -> GPIO7    AIN2 -> GPIO6    PWMA -> GPIO10
 * B 通道（风扇2）:
 *   BIN1 -> GPIO41   BIN2 -> GPIO42   PWMB -> GPIO48
 */
#define MOTOR_AIN1_GPIO     7
#define MOTOR_AIN2_GPIO     6
#define MOTOR_PWMA_GPIO     10

#define MOTOR_BIN1_GPIO     41
#define MOTOR_BIN2_GPIO     42
#define MOTOR_PWMB_GPIO     48

/* 20 档调速，每档 5% 占空比 */
#define MOTOR_SPEED_MAX_LEVEL   20
#define MOTOR_SPEED_STEP_PERCENT 5

/* 电机通道 */
typedef enum {
    MOTOR_CHANNEL_A = 0,   /* 风扇1 */
    MOTOR_CHANNEL_B,       /* 风扇2 */
    MOTOR_CHANNEL_MAX
} motor_channel_t;

/**
 * @brief 初始化 TB6612 双通道电机驱动（GPIO + LEDC PWM）。
 */
void motor_init(void);

/**
 * @brief 反初始化电机驱动。
 */
void motor_deinit(void);

/**
 * @brief 设置风扇A转速档位（0 ~ MOTOR_SPEED_MAX_LEVEL）。
 *        兼容旧接口，等价于 motor_set_speed_ch(MOTOR_CHANNEL_A, level)。
 * @param level 0 为关闭，20 为全速。
 */
void motor_set_speed(uint8_t level);

/**
 * @brief 设置指定通道风扇转速档位。
 * @param ch 电机通道
 * @param level 0 为关闭，20 为全速。
 */
void motor_set_speed_ch(motor_channel_t ch, uint8_t level);

/**
 * @brief 获取风扇A当前转速档位。
 */
uint8_t motor_get_speed(void);

/**
 * @brief 获取指定通道风扇转速档位。
 */
uint8_t motor_get_speed_ch(motor_channel_t ch);

/**
 * @brief 打开风扇A（恢复到上次档位；若上次为 0 则全速）。
 */
void motor_on(void);

/**
 * @brief 打开指定通道风扇。
 */
void motor_on_ch(motor_channel_t ch);

/**
 * @brief 关闭风扇A（档位设为 0）。
 */
void motor_off(void);

/**
 * @brief 关闭指定通道风扇。
 */
void motor_off_ch(motor_channel_t ch);

/**
 * @brief 切换风扇A开关状态。
 */
void motor_toggle(void);

/**
 * @brief 切换指定通道风扇开关状态。
 */
void motor_toggle_ch(motor_channel_t ch);

/**
 * @brief 返回风扇A是否处于开启状态（档位 > 0）。
 */
bool motor_state(void);

/**
 * @brief 返回指定通道风扇是否处于开启状态。
 */
bool motor_state_ch(motor_channel_t ch);

#endif /* __MOTOR_H */
