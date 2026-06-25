#include "state.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "ble_hr.h"

static const char *TAG = "RULES_STATE";

/* ============== 可调阈值 ============== */

/* 佩戴判断 */
#define WEARING_LOST_TIMEOUT_MS     10000  /* 超过 10s 无有效心率认为未佩戴 */

/* 睡眠判断：人在 + 环境暗 + 心率低且稳定 + 持续一段时间 */
#define SLEEP_LIGHT_LUX_THRESHOLD   50     /* lux */
#define SLEEP_HR_MIN                50     /* bpm */
#define SLEEP_HR_MAX                75     /* bpm */
#define SLEEP_HR_STD_MAX            5.0f   /* bpm */
#define SLEEP_CONFIRM_PERIOD_MS     30000  /* 持续 30s 才判定入睡 */
#define WAKE_CONFIRM_PERIOD_MS      15000  /* 持续 15s 不满足睡眠条件则判定醒来 */

/* 心率异常判断 */
#define HR_FAST_THRESHOLD           120    /* bpm */
#define HR_SLOW_THRESHOLD           50     /* bpm */
#define HR_BASELINE_DEVIATION_PCT   20     /* % */
#define HR_HISTORY_SIZE             30     /* 15s @ 500ms */
#define HR_HISTORY_MIN_FILLED       10     /* 至少 10 个有效点才计算基线 */

/* ============== 内部状态 ============== */

static wearing_state_t  s_wearing  = WEARING_UNKNOWN;
static sleeping_state_t s_sleeping = SLEEPING_UNKNOWN;
static hr_state_t       s_hr       = HR_NORMAL;

/* 心率历史环形缓冲区，用于计算基线与稳定性 */
static uint8_t  s_hr_history[HR_HISTORY_SIZE];
static uint8_t  s_hr_count = 0;
static uint8_t  s_hr_index = 0;

/* 上次收到有效心率的时间戳 */
static int64_t  s_last_hr_time_us = 0;

/* 睡眠/清醒状态累计计时 */
static int64_t  s_sleep_match_start_us = 0;
static int64_t  s_wake_match_start_us  = 0;
static bool     s_sleep_condition_met  = false;

/* ============== 辅助函数 ============== */

static int64_t rules_now_us(void)
{
    return esp_timer_get_time();
}

static void rules_hr_history_push(uint8_t hr)
{
    if (hr == 0) {
        return;
    }
    s_hr_history[s_hr_index] = hr;
    s_hr_index = (s_hr_index + 1) % HR_HISTORY_SIZE;
    if (s_hr_count < HR_HISTORY_SIZE) {
        s_hr_count++;
    }
    s_last_hr_time_us = rules_now_us();
}

static void rules_hr_stats(float *out_mean, float *out_std)
{
    *out_mean = 0.0f;
    *out_std  = 0.0f;

    if (s_hr_count == 0) {
        return;
    }

    int sum = 0;
    for (int i = 0; i < s_hr_count; i++) {
        sum += s_hr_history[i];
    }
    float mean = (float)sum / (float)s_hr_count;

    float var_sum = 0.0f;
    for (int i = 0; i < s_hr_count; i++) {
        float diff = (float)s_hr_history[i] - mean;
        var_sum += diff * diff;
    }
    float std = sqrtf(var_sum / (float)s_hr_count);

    *out_mean = mean;
    *out_std  = std;
}

static bool rules_hr_stable_for_sleep(void)
{
    float mean, std;
    rules_hr_stats(&mean, &std);

    if (s_hr_count < HR_HISTORY_MIN_FILLED) {
        return false;
    }

    return (mean >= SLEEP_HR_MIN && mean <= SLEEP_HR_MAX && std <= SLEEP_HR_STD_MAX);
}

/* ============== 状态判断 ============== */

static void rules_update_wearing(const system_state_t *state)
{
    bool connected = ble_hr_is_connected();
    bool has_hr    = (state->heart_rate > 0);
    int64_t now    = rules_now_us();

    /* 已连接且有数据 -> 佩戴 */
    if (connected && has_hr) {
        s_wearing = WEARING_YES;
        return;
    }

    /* 未连接或长时间无数据 -> 未佩戴 */
    if (!connected) {
        s_wearing = WEARING_NOT;
        return;
    }

    /* 已连接但心率丢失 */
    if (s_last_hr_time_us == 0) {
        s_wearing = WEARING_UNKNOWN;
        return;
    }

    int64_t elapsed_ms = (now - s_last_hr_time_us) / 1000;
    if (elapsed_ms >= WEARING_LOST_TIMEOUT_MS) {
        s_wearing = WEARING_NOT;
    } else if (s_wearing == WEARING_UNKNOWN) {
        /* 刚连接但还没收到心率，保守认为未佩戴 */
        s_wearing = WEARING_NOT;
    }
}

static void rules_update_hr_abnormal(const system_state_t *state)
{
    uint8_t hr = state->heart_rate;

    if (hr == 0) {
        s_hr = HR_NORMAL;
        return;
    }

    /* 心动过速 / 心动过缓 */
    if (hr > HR_FAST_THRESHOLD) {
        s_hr = HR_ABNORMAL_FAST;
        return;
    }
    if (hr < HR_SLOW_THRESHOLD) {
        s_hr = HR_ABNORMAL_SLOW;
        return;
    }

    /* 与基线比较：波动超过阈值认为不稳定 */
    float mean, std;
    rules_hr_stats(&mean, &std);
    (void)std;

    if (s_hr_count >= HR_HISTORY_MIN_FILLED && mean > 0.0f) {
        float deviation_pct = fabsf((float)hr - mean) / mean * 100.0f;
        if (deviation_pct > HR_BASELINE_DEVIATION_PCT) {
            s_hr = HR_ABNORMAL_UNSTABLE;
            return;
        }
    }

    s_hr = HR_NORMAL;
}

static void rules_update_sleeping(const system_state_t *state)
{
    /* 睡眠条件：人在 + 环境暗 + 心率低且稳定 */
    bool condition = state->human_present
                  && (state->light_lx < SLEEP_LIGHT_LUX_THRESHOLD)
                  && rules_hr_stable_for_sleep();

    int64_t now = rules_now_us();

    if (condition) {
        if (!s_sleep_condition_met) {
            s_sleep_condition_met = true;
            s_sleep_match_start_us = now;
            s_wake_match_start_us = 0;
        }
        if (s_sleeping != SLEEPING_YES) {
            int64_t elapsed_ms = (now - s_sleep_match_start_us) / 1000;
            if (elapsed_ms >= SLEEP_CONFIRM_PERIOD_MS) {
                s_sleeping = SLEEPING_YES;
                ESP_LOGI(TAG, "sleep detected: hr_mean stable, lux=%d, pir=%d",
                         state->light_lx, state->human_present);
            }
        }
    } else {
        if (s_sleep_condition_met) {
            s_sleep_condition_met = false;
            s_wake_match_start_us = now;
            s_sleep_match_start_us = 0;
        }
        if (s_sleeping != SLEEPING_NOT) {
            int64_t elapsed_ms = s_wake_match_start_us > 0
                               ? (now - s_wake_match_start_us) / 1000
                               : 0;
            if (elapsed_ms >= WAKE_CONFIRM_PERIOD_MS) {
                s_sleeping = SLEEPING_NOT;
                ESP_LOGI(TAG, "wake detected");
            }
        }
    }
}

/* ============== 对外接口 ============== */

void rules_state_init(void)
{
    s_wearing  = WEARING_UNKNOWN;
    s_sleeping = SLEEPING_UNKNOWN;
    s_hr       = HR_NORMAL;

    memset(s_hr_history, 0, sizeof(s_hr_history));
    s_hr_count = 0;
    s_hr_index = 0;
    s_last_hr_time_us = 0;

    s_sleep_condition_met  = false;
    s_sleep_match_start_us = 0;
    s_wake_match_start_us  = 0;

    ESP_LOGI(TAG, "rules state engine init ok");
}

void rules_state_update(system_state_t *state)
{
    if (state == NULL) {
        return;
    }

    rules_hr_history_push(state->heart_rate);
    rules_update_wearing(state);
    rules_update_hr_abnormal(state);
    rules_update_sleeping(state);

    state->wearing    = (s_wearing == WEARING_YES);
    state->sleeping   = (s_sleeping == SLEEPING_YES);
    state->hr_abnormal = (s_hr != HR_NORMAL);

    ESP_LOGI(TAG, "state: wearing=%d, sleeping=%d, hr=%d, hr_abnormal=%d",
             s_wearing, s_sleeping, s_hr, state->hr_abnormal);
}

wearing_state_t rules_state_get_wearing(void)
{
    return s_wearing;
}

sleeping_state_t rules_state_get_sleeping(void)
{
    return s_sleeping;
}

hr_state_t rules_state_get_hr(void)
{
    return s_hr;
}
