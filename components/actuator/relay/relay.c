#include "relay.h"
#include "driver/gpio.h"

void relay_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    relay_off();
}

void relay_on(void)
{
    gpio_set_level(RELAY_GPIO, 1);
}

void relay_off(void)
{
    gpio_set_level(RELAY_GPIO, 0);
}

void relay_toggle(void)
{
    gpio_set_level(RELAY_GPIO, !gpio_get_level(RELAY_GPIO));
}

bool relay_state(void)
{
    return gpio_get_level(RELAY_GPIO) != 0;
}
