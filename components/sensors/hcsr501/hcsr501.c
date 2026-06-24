#include "hcsr501.h"
#include "driver/gpio.h"

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
}

bool hcsr501_detected(void)
{
    /* HC-SR501 输出高电平表示检测到人体 */
    return gpio_get_level(HCSR501_GPIO) == 1;
}
