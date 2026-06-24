#include "motor.h"
#include "driver/gpio.h"

void motor_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    motor_off();
}

void motor_on(void)
{
    gpio_set_level(MOTOR_GPIO, 1);
}

void motor_off(void)
{
    gpio_set_level(MOTOR_GPIO, 0);
}

void motor_toggle(void)
{
    gpio_set_level(MOTOR_GPIO, !gpio_get_level(MOTOR_GPIO));
}

bool motor_state(void)
{
    return gpio_get_level(MOTOR_GPIO) != 0;
}
