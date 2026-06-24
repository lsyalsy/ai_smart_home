/*
 * 1.8 寸 TFT (ST7735R/S) 驱动
 * 从 STM32 参考代码移植到 ESP32-S3 (ESP-IDF)
 * 使用软件 SPI（bit-bang），便于调试和 pin 脚灵活分配
 */
#include "st7735.h"
#include "font.h"
#include "chinese_font.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 毫秒延时 */
static void tft_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* GPIO 输出宏 */
static inline void tft_sck(int level)  { gpio_set_level(TFT_SCK_GPIO,  level); }
static inline void tft_mosi(int level) { gpio_set_level(TFT_MOSI_GPIO, level); }
static inline void tft_cs(int level)   { gpio_set_level(TFT_CS_GPIO,   level); }
static inline void tft_dc(int level)   { gpio_set_level(TFT_DC_GPIO,   level); }
static inline void tft_rst(int level)  { gpio_set_level(TFT_RST_GPIO,  level); }

/* 软件 SPI 发送 8 bit */
static void tft_spi_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        tft_mosi((data & 0x80) ? 1 : 0);
        tft_sck(0);
        tft_sck(1);
        data <<= 1;
    }
}

/* 写命令 */
static void tft_write_index(uint8_t index)
{
    tft_cs(0);
    tft_dc(0);
    tft_spi_write_byte(index);
    tft_cs(1);
}

/* 写数据 */
static void tft_write_data(uint8_t data)
{
    tft_cs(0);
    tft_dc(1);
    tft_spi_write_byte(data);
    tft_cs(1);
}

/* 写 16 bit 数据（颜色） */
static void tft_write_data_16bit(uint16_t data)
{
    tft_cs(0);
    tft_dc(1);
    tft_spi_write_byte(data >> 8);
    tft_spi_write_byte(data & 0xFF);
    tft_cs(1);
}

/* 复位 */
static void tft_reset(void)
{
    tft_rst(0);
    tft_delay_ms(100);
    tft_rst(1);
    tft_delay_ms(50);
}

/* GPIO 初始化 */
void tft_gpio_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    io_conf.pin_bit_mask = (1ULL << TFT_SCK_GPIO)
                         | (1ULL << TFT_MOSI_GPIO)
                         | (1ULL << TFT_CS_GPIO)
                         | (1ULL << TFT_DC_GPIO)
                         | (1ULL << TFT_RST_GPIO)
                         | (1ULL << TFT_BL_GPIO);
    gpio_config(&io_conf);

    tft_sck(0);
    tft_mosi(0);
    tft_cs(1);
    tft_dc(1);
    tft_rst(1);
    tft_backlight_on();
}

/* 背光控制 */
void tft_backlight_on(void)
{
    gpio_set_level(TFT_BL_GPIO, 1);
}

void tft_backlight_off(void)
{
    gpio_set_level(TFT_BL_GPIO, 0);
}

/* ST7735R 初始化序列（与参考代码一致） */
void tft_init(void)
{
    tft_gpio_init();
    tft_reset();

    tft_write_index(0x11); /* Sleep exit */
    tft_delay_ms(120);

    /* Frame Rate */
    tft_write_index(0xB1);
    tft_write_data(0x01);
    tft_write_data(0x2C);
    tft_write_data(0x2D);

    tft_write_index(0xB2);
    tft_write_data(0x01);
    tft_write_data(0x2C);
    tft_write_data(0x2D);

    tft_write_index(0xB3);
    tft_write_data(0x01);
    tft_write_data(0x2C);
    tft_write_data(0x2D);
    tft_write_data(0x01);
    tft_write_data(0x2C);
    tft_write_data(0x2D);

    tft_write_index(0xB4); /* Column inversion */
    tft_write_data(0x07);

    /* Power Sequence */
    tft_write_index(0xC0);
    tft_write_data(0xA2);
    tft_write_data(0x02);
    tft_write_data(0x84);

    tft_write_index(0xC1);
    tft_write_data(0xC5);

    tft_write_index(0xC2);
    tft_write_data(0x0A);
    tft_write_data(0x00);

    tft_write_index(0xC3);
    tft_write_data(0x8A);
    tft_write_data(0x2A);

    tft_write_index(0xC4);
    tft_write_data(0x8A);
    tft_write_data(0xEE);

    tft_write_index(0xC5); /* VCOM */
    tft_write_data(0x0E);

    tft_write_index(0x36); /* MX, MY, RGB mode */
    tft_write_data(0xC0);

    /* Gamma Sequence */
    tft_write_index(0xE0);
    tft_write_data(0x0f); tft_write_data(0x1a); tft_write_data(0x0f);
    tft_write_data(0x18); tft_write_data(0x2f); tft_write_data(0x28);
    tft_write_data(0x20); tft_write_data(0x22); tft_write_data(0x1f);
    tft_write_data(0x1b); tft_write_data(0x23); tft_write_data(0x37);
    tft_write_data(0x00); tft_write_data(0x07); tft_write_data(0x02);
    tft_write_data(0x10);

    tft_write_index(0xE1);
    tft_write_data(0x0f); tft_write_data(0x1b); tft_write_data(0x0f);
    tft_write_data(0x17); tft_write_data(0x33); tft_write_data(0x2c);
    tft_write_data(0x29); tft_write_data(0x2e); tft_write_data(0x30);
    tft_write_data(0x30); tft_write_data(0x39); tft_write_data(0x3f);
    tft_write_data(0x00); tft_write_data(0x07); tft_write_data(0x03);
    tft_write_data(0x10);

    tft_write_index(0x2A);
    tft_write_data(0x00); tft_write_data(0x00);
    tft_write_data(0x00); tft_write_data(0x7F);

    tft_write_index(0x2B);
    tft_write_data(0x00); tft_write_data(0x00);
    tft_write_data(0x00); tft_write_data(0x9F);

    tft_write_index(0xF0); /* Enable test command */
    tft_write_data(0x01);

    tft_write_index(0xF6); /* Disable ram power save mode */
    tft_write_data(0x00);

    tft_write_index(0x3A); /* 65k mode */
    tft_write_data(0x05);

    tft_write_index(0x29); /* Display on */
}

/* 设置显示区域 */
void tft_set_region(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    tft_write_index(0x2A);
    tft_write_data(0x00);
    tft_write_data(x_start);
    tft_write_data(0x00);
    tft_write_data(x_end);

    tft_write_index(0x2B);
    tft_write_data(0x00);
    tft_write_data(y_start);
    tft_write_data(0x00);
    tft_write_data(y_end);

    tft_write_index(0x2C);
}

/* 清屏 */
void tft_clear(uint16_t color)
{
    tft_set_region(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    for (uint16_t i = 0; i < TFT_WIDTH; i++) {
        for (uint16_t j = 0; j < TFT_HEIGHT; j++) {
            tft_write_data_16bit(color);
        }
    }
}

/* 画点 */
void tft_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    tft_set_region(x, y, x, y);
    tft_write_data_16bit(color);
}

/* Bresenham 画线 */
void tft_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int dx = (int)x1 - (int)x0;
    int dy = (int)y1 - (int)y0;
    int x_inc = (dx >= 0) ? 1 : -1;
    int y_inc = (dy >= 0) ? 1 : -1;
    dx = (dx >= 0) ? dx : -dx;
    dy = (dy >= 0) ? dy : -dy;
    int dx2 = dx << 1;
    int dy2 = dy << 1;

    int x = x0;
    int y = y0;

    tft_draw_point(x, y, color);

    if (dx > dy) {
        int err = dy2 - dx;
        for (int i = 0; i <= dx; i++) {
            if (err >= 0) {
                err -= dx2;
                y += y_inc;
            }
            err += dy2;
            x += x_inc;
            tft_draw_point(x, y, color);
        }
    } else {
        int err = dx2 - dy;
        for (int i = 0; i <= dy; i++) {
            if (err >= 0) {
                err -= dy2;
                x += x_inc;
            }
            err += dx2;
            y += y_inc;
            tft_draw_point(x, y, color);
        }
    }
}

/* 画矩形框 */
void tft_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    tft_draw_line(x, y, x + w - 1, y, color);
    tft_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    tft_draw_line(x + w - 1, y + h - 1, x, y + h - 1, color);
    tft_draw_line(x, y + h - 1, x, y, color);
}

/* 填充矩形 */
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    tft_set_region(x, y, x + w - 1, y + h - 1);
    for (uint16_t i = 0; i < w * h; i++) {
        tft_write_data_16bit(color);
    }
}

/* 显示一个 ASCII 字符（size 目前只支持 16） */
void tft_show_char(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc, uint8_t size)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (ch < ' ' || ch > '~') ch = ' ';

    uint8_t idx = ch - ' ';

    for (uint8_t row = 0; row < 16; row++) {
        uint8_t byte = asc16[idx][row];
        for (uint8_t col = 0; col < 8; col++) {
            uint16_t px = x + col;
            uint16_t py = y + row;
            if (px >= TFT_WIDTH || py >= TFT_HEIGHT) continue;
            tft_draw_point(px, py, (byte & (0x80 >> col)) ? fc : bc);
        }
    }
}

/* 显示字符串 */
void tft_show_string(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc, uint8_t size)
{
    uint16_t x_offset = x;
    while (*str) {
        tft_show_char(x_offset, y, *str, fc, bc, size);
        x_offset += 8; /* 字宽 8 像素 */
        str++;
    }
}

/* 显示整数 */
void tft_show_num(uint16_t x, uint16_t y, int32_t num, uint16_t fc, uint16_t bc, uint8_t size)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "%ld", num);
    tft_show_string(x, y, buf, fc, bc, size);
}

/* 显示一个 16x16 中文字符（utf8_ch 指向 3 字节 UTF-8 序列） */
void tft_show_chn_char(uint16_t x, uint16_t y, const char *utf8_ch, uint16_t fc, uint16_t bc)
{
    int idx = -1;
    const char *p = chn_font_index;

    for (int i = 0; i < CHN_FONT_NUM; i++) {
        if (p[0] == utf8_ch[0] && p[1] == utf8_ch[1] && p[2] == utf8_ch[2]) {
            idx = i;
            break;
        }
        p += 3;
    }

    if (idx < 0) return; /* 字库中没有该字符 */

    const uint8_t *bitmap = chn_font_bitmap[idx];
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            uint8_t byte = (col < 8) ? bitmap[row] : bitmap[row + 16];
            uint8_t bit = (col < 8) ? (0x80 >> col) : (0x80 >> (col - 8));
            tft_draw_point(x + col, y + row, (byte & bit) ? fc : bc);
        }
    }
}

/* 显示中英混合字符串（ASCII 8x16，中文 16x16） */
void tft_show_chn_string(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc)
{
    uint16_t x_offset = x;
    while (*str) {
        if ((*str & 0x80) == 0) {
            /* ASCII */
            tft_show_char(x_offset, y, *str, fc, bc, 16);
            x_offset += 8;
            str++;
        } else if ((*str & 0xF0) == 0xE0) {
            /* 3 字节 UTF-8：中文 */
            tft_show_chn_char(x_offset, y, str, fc, bc);
            x_offset += 16;
            str += 3;
        } else {
            /* 其他 UTF-8 暂不处理，跳过 */
            str++;
        }
    }
}
