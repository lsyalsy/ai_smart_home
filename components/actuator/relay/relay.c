#include "relay.h"
#include "driver/gpio.h"

static const gpio_num_t s_relay_gpios[RELAY_CHANNEL_MAX] = {
    RELAY_HUMIDIFIER_GPIO,
    RELAY_BATHROOM_GPIO
};

void relay_init(void)
{
    uint64_t pin_mask = 0;
    for (int i = 0; i < RELAY_CHANNEL_MAX; i++) {
        pin_mask |= (1ULL << s_relay_gpios[i]);
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    for (int i = 0; i < RELAY_CHANNEL_MAX; i++) {
        gpio_set_level(s_relay_gpios[i], 0);
    }
}

void relay_set(relay_channel_t ch, bool on)
{
    if (ch < 0 || ch >= RELAY_CHANNEL_MAX) {
        return;
    }
    gpio_set_level(s_relay_gpios[ch], on ? 1 : 0);
}

bool relay_get(relay_channel_t ch)
{
    if (ch < 0 || ch >= RELAY_CHANNEL_MAX) {
        return false;
    }
    return gpio_get_level(s_relay_gpios[ch]) != 0;
}

void relay_toggle(relay_channel_t ch)
{
    if (ch < 0 || ch >= RELAY_CHANNEL_MAX) {
        return;
    }
    gpio_set_level(s_relay_gpios[ch], !gpio_get_level(s_relay_gpios[ch]));
}

/* 兼容旧接口：默认操作加湿器 */
void relay_on(void)
{
    relay_set(RELAY_CHANNEL_HUMIDIFIER, true);
}

void relay_off(void)
{
    relay_set(RELAY_CHANNEL_HUMIDIFIER, false);
}

void relay_humidifier_toggle(void)
{
    relay_toggle(RELAY_CHANNEL_HUMIDIFIER);
}

bool relay_state(void)
{
    return relay_get(RELAY_CHANNEL_HUMIDIFIER);
}
