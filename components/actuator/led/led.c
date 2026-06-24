#include "led.h"
#include "driver/gpio.h"

void led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    led_off();
}

void led_on(void)
{
    gpio_set_level(LED_GPIO, 1);
}

void led_off(void)
{
    gpio_set_level(LED_GPIO, 0);
}

void led_toggle(void)
{
    gpio_set_level(LED_GPIO, !gpio_get_level(LED_GPIO));
}

bool led_state(void)
{
    return gpio_get_level(LED_GPIO) != 0;
}
