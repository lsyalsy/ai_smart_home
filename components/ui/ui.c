/*
 * TFT UI 页面渲染
 * 数据页、建议页、状态页、闹钟页
 *
 * 刷新策略：
 *  - ui_render_page：切换页面时调用，清屏并重绘标题栏 + 内容。
 *  - ui_update_page：同页数据变化时调用，只重绘数值/状态区域，不清屏。
 */
#include "ui.h"
#include "st7735.h"
#include <stdio.h>
#include <string.h>

#define TITLE_BAR_H  20
#define CONTENT_Y    22

/* 数值显示区域：标签右侧 72~128 像素，宽 56 像素 */
#define VALUE_X      72
#define VALUE_W      (TFT_WIDTH - VALUE_X)

/* 页面标题 */
static const char *page_titles[PAGE_MAX] = {
    [PAGE_DATA]       = "第一页 数据",
    [PAGE_SUGGESTION] = "第二页 建议",
    [PAGE_STATUS]     = "第三页 状态",
    [PAGE_ALARM]      = "第四页 闹钟",
};

static ui_page_t s_current_page = PAGE_DATA;

const char *ui_page_title(ui_page_t page)
{
    if (page >= PAGE_MAX) return "";
    return page_titles[page];
}

ui_page_t ui_get_current_page(void)
{
    return s_current_page;
}

/* 计算中英混合字符串的像素宽度 */
static int text_width_pixels(const char *str)
{
    int width = 0;
    while (*str) {
        if ((*str & 0x80) == 0) {
            width += 8;  /* ASCII */
            str++;
        } else if ((*str & 0xF0) == 0xE0) {
            width += 16; /* 中文 */
            str += 3;
        } else {
            str++;
        }
    }
    return width;
}

/* 绘制顶部标题栏 */
static void draw_title_bar(const char *title)
{
    tft_fill_rect(0, 0, TFT_WIDTH, TITLE_BAR_H, COLOR_BLUE);
    int title_width = text_width_pixels(title);
    int x = (TFT_WIDTH - title_width) / 2;
    if (x < 0) x = 0;
    /* 标题文字在 20px 高中居中：16px 字高，上下各 2px 边距 */
    tft_show_chn_string(x, 2, title, COLOR_WHITE, COLOR_BLUE);
}

/* 用背景色清除数值显示区域，再绘制新值 */
static void draw_value(int y, const char *value, uint16_t value_color)
{
    tft_fill_rect(VALUE_X, y, VALUE_W, 16, COLOR_BLACK);
    tft_show_string(VALUE_X, y, value, value_color, COLOR_BLACK, 16);
}

/* 用背景色清除状态显示区域，再绘制中文状态 */
static void draw_state(int y, const char *state, uint16_t state_color)
{
    tft_fill_rect(VALUE_X, y, VALUE_W, 16, COLOR_BLACK);
    tft_show_chn_string(VALUE_X, y, state, state_color, COLOR_BLACK);
}

/* 绘制一行标签 + 数值 */
static void draw_label_value(int y, const char *label, const char *value, uint16_t value_color)
{
    tft_show_chn_string(4, y, label, COLOR_WHITE, COLOR_BLACK);
    draw_value(y, value, value_color);
}

/* 绘制一行标签 + 状态 */
static void draw_label_state(int y, const char *label, const char *state, uint16_t state_color)
{
    tft_show_chn_string(4, y, label, COLOR_WHITE, COLOR_BLACK);
    draw_state(y, state, state_color);
}

/* ========== 数据页 ========== */
static void page_data_render(const system_state_t *s)
{
    char buf[24];
    int y = CONTENT_Y;

    snprintf(buf, sizeof(buf), "%.1f C", s->temperature);
    draw_label_value(y, "温度", buf, COLOR_GREEN); y += 20;

    snprintf(buf, sizeof(buf), "%.1f %%", s->humidity);
    draw_label_value(y, "湿度", buf, COLOR_CYAN); y += 20;

    snprintf(buf, sizeof(buf), "%d lx", s->light_lx);
    draw_label_value(y, "光照", buf, COLOR_YELLOW); y += 20;

    draw_label_state(y, "人体", s->human_present ? "有人" : "无人",
                     s->human_present ? COLOR_GREEN : COLOR_GRAY); y += 20;

    draw_label_state(y, "烟雾", s->smoke_alarm ? "有" : "无",
                     s->smoke_alarm ? COLOR_RED : COLOR_GREEN); y += 20;

    snprintf(buf, sizeof(buf), "%d bpm", s->heart_rate);
    draw_label_value(y, "心率", buf, s->heart_rate > 120 ? COLOR_RED : COLOR_MAGENTA);
}

/* 数据页局部刷新（只刷新数值/状态区） */
static void page_data_update(const system_state_t *s)
{
    char buf[24];
    int y = CONTENT_Y;

    snprintf(buf, sizeof(buf), "%.1f C", s->temperature);
    draw_value(y, buf, COLOR_GREEN); y += 20;

    snprintf(buf, sizeof(buf), "%.1f %%", s->humidity);
    draw_value(y, buf, COLOR_CYAN); y += 20;

    snprintf(buf, sizeof(buf), "%d lx", s->light_lx);
    draw_value(y, buf, COLOR_YELLOW); y += 20;

    draw_state(y, s->human_present ? "有人" : "无人",
               s->human_present ? COLOR_GREEN : COLOR_GRAY); y += 20;

    draw_state(y, s->smoke_alarm ? "有" : "无",
               s->smoke_alarm ? COLOR_RED : COLOR_GREEN); y += 20;

    snprintf(buf, sizeof(buf), "%d bpm", s->heart_rate);
    draw_value(y, buf, s->heart_rate > 120 ? COLOR_RED : COLOR_MAGENTA);
}

/* ========== 建议页 ========== */
static void page_suggestion_render(const system_state_t *s)
{
    /* 建议页目前只有文本，直接重绘即可 */
    draw_wrapped_suggestion(s->suggestion);
}

/* 自动换行显示中文建议，每行最多 7 个汉字（128/16=8，留边距） */
static void draw_wrapped_suggestion(const char *text)
{
    int x = 4;
    int y = CONTENT_Y;
    const char *p = text;
    int chars_this_line = 0;

    while (*p && y < TFT_HEIGHT - 16) {
        if ((*p & 0xF0) == 0xE0) {
            /* 3 字节 UTF-8 中文 */
            if (chars_this_line >= 7) {
                x = 4;
                y += 20;
                chars_this_line = 0;
            }
            tft_show_chn_char(x, y, p, COLOR_YELLOW, COLOR_BLACK);
            x += 16;
            p += 3;
            chars_this_line++;
        } else if ((*p & 0x80) == 0) {
            /* ASCII */
            if (x + 8 > TFT_WIDTH - 4) {
                x = 4;
                y += 20;
                chars_this_line = 0;
            }
            tft_show_char(x, y, *p, COLOR_YELLOW, COLOR_BLACK, 16);
            x += 8;
            p++;
        } else {
            p++;
        }
    }
}

/* ========== 状态页 ========== */
static void page_status_render(const system_state_t *s)
{
    int y = CONTENT_Y;

    draw_label_state(y, "灯光", s->led_on ? "打开" : "关闭",
                     s->led_on ? COLOR_YELLOW : COLOR_GRAY); y += 24;

    const char *fan_str = "关闭";
    uint16_t fan_color = COLOR_GRAY;
    switch (s->fan_mode) {
        case FAN_MODE_MANUAL: fan_str = "手动"; fan_color = COLOR_CYAN; break;
        case FAN_MODE_AUTO:   fan_str = "自动"; fan_color = COLOR_GREEN; break;
        default: break;
    }
    draw_label_state(y, "风扇", fan_str, fan_color); y += 24;

    draw_label_state(y, "加湿器", s->humidifier_on ? "打开" : "关闭",
                     s->humidifier_on ? COLOR_BLUE : COLOR_GRAY); y += 24;

    draw_label_state(y, "报警", s->alarm_triggered ? "异常" : "正常",
                     s->alarm_triggered ? COLOR_RED : COLOR_GREEN);
}

/* 状态页局部刷新 */
static void page_status_update(const system_state_t *s)
{
    int y = CONTENT_Y;

    draw_state(y, s->led_on ? "打开" : "关闭",
               s->led_on ? COLOR_YELLOW : COLOR_GRAY); y += 24;

    const char *fan_str = "关闭";
    uint16_t fan_color = COLOR_GRAY;
    switch (s->fan_mode) {
        case FAN_MODE_MANUAL: fan_str = "手动"; fan_color = COLOR_CYAN; break;
        case FAN_MODE_AUTO:   fan_str = "自动"; fan_color = COLOR_GREEN; break;
        default: break;
    }
    draw_state(y, fan_str, fan_color); y += 24;

    draw_state(y, s->humidifier_on ? "打开" : "关闭",
               s->humidifier_on ? COLOR_BLUE : COLOR_GRAY); y += 24;

    draw_state(y, s->alarm_triggered ? "异常" : "正常",
               s->alarm_triggered ? COLOR_RED : COLOR_GREEN);
}

/* ========== 闹钟页 ========== */
static void page_alarm_render(const system_state_t *s)
{
    char buf[32];
    int y = CONTENT_Y;

    /* 使用冒号分隔的单行显示，避免状态值过长超出屏幕 */
    snprintf(buf, sizeof(buf), "时间: %02d:%02d", s->alarm_hour, s->alarm_minute);
    tft_show_chn_string(4, y, buf, COLOR_YELLOW, COLOR_BLACK); y += 26;

    if (s->alarm_mode == ALARM_MODE_LIGHT) {
        tft_show_chn_string(4, y, "模式: 渐变灯光", COLOR_CYAN, COLOR_BLACK);
    } else {
        tft_show_chn_string(4, y, "模式: 灯光蜂鸣", COLOR_CYAN, COLOR_BLACK);
    }
    y += 26;

    snprintf(buf, sizeof(buf), "状态: %s", s->alarm_enabled ? "已开启" : "已关闭");
    tft_show_chn_string(4, y, buf, s->alarm_enabled ? COLOR_GREEN : COLOR_GRAY, COLOR_BLACK);

    /* 底部小提示 */
    tft_show_chn_string(4, TFT_HEIGHT - 16, "Web端可设置", COLOR_GRAY, COLOR_BLACK);
}

/* 闹钟页局部刷新 */
static void page_alarm_update(const system_state_t *s)
{
    char buf[32];
    int y = CONTENT_Y;

    /* 时间行：清除整行后重绘 */
    tft_fill_rect(4, y, TFT_WIDTH - 8, 16, COLOR_BLACK);
    snprintf(buf, sizeof(buf), "时间: %02d:%02d", s->alarm_hour, s->alarm_minute);
    tft_show_chn_string(4, y, buf, COLOR_YELLOW, COLOR_BLACK); y += 26;

    /* 模式行：清除整行后重绘 */
    tft_fill_rect(4, y, TFT_WIDTH - 8, 16, COLOR_BLACK);
    if (s->alarm_mode == ALARM_MODE_LIGHT) {
        tft_show_chn_string(4, y, "模式: 渐变灯光", COLOR_CYAN, COLOR_BLACK);
    } else {
        tft_show_chn_string(4, y, "模式: 灯光蜂鸣", COLOR_CYAN, COLOR_BLACK);
    }
    y += 26;

    /* 状态行：清除整行后重绘 */
    tft_fill_rect(4, y, TFT_WIDTH - 8, 16, COLOR_BLACK);
    snprintf(buf, sizeof(buf), "状态: %s", s->alarm_enabled ? "已开启" : "已关闭");
    tft_show_chn_string(4, y, buf, s->alarm_enabled ? COLOR_GREEN : COLOR_GRAY, COLOR_BLACK);
}

/* 对外渲染接口：切换页面时调用 */
void ui_render_page(ui_page_t page, const system_state_t *state)
{
    if (page >= PAGE_MAX || state == NULL) return;

    s_current_page = page;
    tft_clear(COLOR_BLACK);
    draw_title_bar(ui_page_title(page));

    switch (page) {
        case PAGE_DATA:       page_data_render(state);       break;
        case PAGE_SUGGESTION: page_suggestion_render(state); break;
        case PAGE_STATUS:     page_status_render(state);     break;
        case PAGE_ALARM:      page_alarm_render(state);      break;
        default: break;
    }
}

/* 对外更新接口：同页数据变化时调用，不清屏 */
void ui_update_page(const system_state_t *state)
{
    if (state == NULL) return;

    switch (s_current_page) {
        case PAGE_DATA:       page_data_update(state);       break;
        case PAGE_SUGGESTION: page_suggestion_render(state); break;
        case PAGE_STATUS:     page_status_update(state);     break;
        case PAGE_ALARM:      page_alarm_update(state);      break;
        default: break;
    }
}
