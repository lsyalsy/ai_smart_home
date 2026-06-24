#include "buzzer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void buzzer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    buzzer_off();
}

void buzzer_on(void)
{
    gpio_set_level(BUZZER_GPIO, 1);
}

void buzzer_off(void)
{
    gpio_set_level(BUZZER_GPIO, 0);
}

void buzzer_toggle(void)
{
    gpio_set_level(BUZZER_GPIO, !gpio_get_level(BUZZER_GPIO));
}

void buzzer_beep(uint32_t ms)
{
    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(ms));
    buzzer_off();
}

bool buzzer_state(void)
{
    return gpio_get_level(BUZZER_GPIO) != 0;
}
