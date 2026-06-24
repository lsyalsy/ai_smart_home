#include "hcsr501.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static bool s_last_state = false;
static int64_t s_last_change_us = 0;

void hcsr501_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << HCSR501_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    s_last_state = false;
    s_last_change_us = esp_timer_get_time();
}

bool hcsr501_detected(void)
{
    /* HC-SR501 输出高电平表示检测到人体 */
    bool raw = (gpio_get_level(HCSR501_GPIO) == 1);
    int64_t now = esp_timer_get_time();

    /* 软件消抖：状态稳定 100ms 才确认 */
    if (raw != s_last_state) {
        if ((now - s_last_change_us) > 100000) {
            s_last_state = raw;
            s_last_change_us = now;
        }
    } else {
        s_last_change_us = now;
    }

    return s_last_state;
}
