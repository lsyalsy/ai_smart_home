#ifndef __UI_H
#define __UI_H

#include <stdint.h>
#include <stdbool.h>

/* 页面编号 */
typedef enum {
    PAGE_DATA = 0,
    PAGE_SUGGESTION,
    PAGE_STATUS,
    PAGE_ALARM,
    PAGE_MAX
} ui_page_t;

/* 风扇模式 */
typedef enum {
    FAN_MODE_OFF = 0,
    FAN_MODE_MANUAL,
    FAN_MODE_AUTO
} fan_mode_t;

/* 闹钟唤醒模式 */
typedef enum {
    ALARM_MODE_LIGHT = 0,
    ALARM_MODE_LIGHT_BUZZER
} alarm_mode_t;

/* 系统状态结构体
 * 后续由 sensors/actuators/rules 等组件填充真实数据
 * 目前 UI 只负责根据该结构体渲染页面
 */
typedef struct {
    /* 传感器数据 */
    float    temperature;   /* 温度 ℃ */
    float    humidity;      /* 湿度 % */
    uint16_t light_lx;      /* 光照 lux */
    bool     human_present; /* 人体存在 */
    bool     smoke_alarm;   /* 烟雾报警 */
    uint8_t  heart_rate;    /* 心率 bpm */

    /* 大模型建议 */
    char suggestion[96];

    /* 执行器状态 */
    bool       led_on;          /* 灯光 */
    fan_mode_t fan_mode;        /* 风扇模式 */
    uint8_t    fan_speed_level; /* 风扇档位 0~20（仅在手动/自动模式有效） */
    bool       humidifier_on;   /* 加湿器 */
    bool       alarm_triggered; /* 报警器 */

    /* 光闹钟 */
    uint8_t      alarm_hour;
    uint8_t      alarm_minute;
    alarm_mode_t alarm_mode;
    bool         alarm_enabled;
} system_state_t;

/* 渲染指定页面（切换页面时调用，会清屏并重绘标题栏） */
void ui_render_page(ui_page_t page, const system_state_t *state);

/* 仅刷新当前页面中变化的数据字段（不清屏）
 * prev 为上一次状态，state 为当前状态；传入 NULL 时效果与 ui_update_page 相同
 */
void ui_update_page_state(const system_state_t *prev, const system_state_t *state);

/* 获取当前页面 */
ui_page_t ui_get_current_page(void);

/* 获取页面标题 */
const char *ui_page_title(ui_page_t page);

#endif /* __UI_H */
