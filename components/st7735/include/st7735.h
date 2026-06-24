#ifndef __ST7735_H
#define __ST7735_H

#include <stdint.h>

/* 1.8 寸 TFT 模块接线（ESP32-S3-WROOM-1 开发板）
 * 本驱动使用软件 SPI（bit-bang），引脚可任意修改
 *
 * 注意：
 *  - GPIO33/GPIO34 在该开发板上没有引出
 *  - GPIO2  项目分配给 MQ-2 烟雾传感器（ADC）
 *  - GPIO4  项目分配给 HC-SR501 人体感应
 *  - GPIO5/6 项目分配给 BH1750 I2C
 *  - GPIO48 板载 RGB LED 占用
 *
 * TFT 模块引脚    ESP32-S3 引脚
 * --------------  -------------
 * VCC             3.3V
 * GND             GND
 * SCL (SCK/CLK)   GPIO1
 * SDA (MOSI/DIN)  GPIO7
 * CS              GPIO20
 * DC (RS)         GPIO21
 * RST             GPIO47
 * BL (背光)       接 3.3V 常亮
 */

#define TFT_SCK_GPIO    1
#define TFT_MOSI_GPIO   7
#define TFT_CS_GPIO     20
#define TFT_DC_GPIO     21
#define TFT_RST_GPIO    47
#define TFT_BL_GPIO     45  /* 背光由硬件接 3.3V，此 GPIO 仅占位 */

#define TFT_WIDTH       128
#define TFT_HEIGHT      160

/* RGB565 常用颜色 */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_GRAY      0x8410

/* 基础接口 */
void tft_gpio_init(void);
void tft_init(void);
void tft_clear(uint16_t color);
void tft_set_region(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void tft_draw_point(uint16_t x, uint16_t y, uint16_t color);
void tft_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void tft_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/* 字符/字符串显示（16 点阵 ASCII） */
void tft_show_char(uint16_t x, uint16_t y, char ch, uint16_t fc, uint16_t bc, uint8_t size);
void tft_show_string(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc, uint8_t size);
void tft_show_num(uint16_t x, uint16_t y, int32_t num, uint16_t fc, uint16_t bc, uint8_t size);

/* 中文字符串显示（16x16 点阵） */
void tft_show_chn_char(uint16_t x, uint16_t y, const char *utf8_ch, uint16_t fc, uint16_t bc);
void tft_show_chn_string(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc);

/* 背光控制 */
void tft_backlight_on(void);
void tft_backlight_off(void);

#endif /* __ST7735_H */
