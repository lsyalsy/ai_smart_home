#ifndef __RULES_STATE_H
#define __RULES_STATE_H

#include <stdint.h>
#include <stdbool.h>

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 佩戴状态 */
typedef enum {
    WEARING_NOT = 0,
    WEARING_YES,
    WEARING_UNKNOWN
} wearing_state_t;

/* 睡眠状态 */
typedef enum {
    SLEEPING_NOT = 0,
    SLEEPING_YES,
    SLEEPING_UNKNOWN
} sleeping_state_t;

/* 心率状态 */
typedef enum {
    HR_NORMAL = 0,
    HR_ABNORMAL_FAST,    /* 心动过速 */
    HR_ABNORMAL_SLOW,    /* 心动过缓 */
    HR_ABNORMAL_UNSTABLE /* 变化剧烈 */
} hr_state_t;

/**
 * @brief 初始化本地规则引擎（状态判断模块）。
 */
void rules_state_init(void);

/**
 * @brief 根据当前系统状态判断佩戴、睡眠、心率异常状态。
 *
 * 调用后会把结果写入 state->wearing / state->sleeping / state->hr_abnormal。
 *
 * @param state 当前系统状态
 */
void rules_state_update(system_state_t *state);

/**
 * @brief 获取最近一次判断的佩戴状态。
 */
wearing_state_t rules_state_get_wearing(void);

/**
 * @brief 获取最近一次判断的睡眠状态。
 */
sleeping_state_t rules_state_get_sleeping(void);

/**
 * @brief 获取最近一次判断的心率状态。
 */
hr_state_t rules_state_get_hr(void);

#ifdef __cplusplus
}
#endif

#endif /* __RULES_STATE_H */
