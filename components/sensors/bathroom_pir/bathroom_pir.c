#include "bathroom_pir.h"
#include "driver/gpio.h"

void bathroom_pir_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BATHROOM_PIR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

bool bathroom_pir_detected(void)
{
    return gpio_get_level(BATHROOM_PIR_GPIO) != 0;
}
