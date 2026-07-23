#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "dht22.h"

#define DHT22_GPIO_NUM      GPIO_NUM_4
#define DHT22_TIMEOUT_US    100

static portMUX_TYPE s_dht22_mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Wait until the data pin reaches the requested logic level.
 *
 * @param level   Target level (0 or 1).
 * @param timeout Maximum time to wait in microseconds.
 *
 * @return true if level reached within timeout, false on timeout.
 */
static bool dht22_wait_level(int level, uint32_t timeout)
{
    for (uint32_t us = 0; us < timeout; us++) {
        if (gpio_get_level(DHT22_GPIO_NUM) == level) {
            return true;
        }
        esp_rom_delay_us(1);
    }
    return false;
}

void dht22_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DHT22_GPIO_NUM),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(DHT22_GPIO_NUM, 1);
}

/**
 * @brief Low-level read of 40 raw bits from DHT22.
 *
 * Must be called inside a critical section to keep timing stable.
 *
 * @param[out] data 5-byte buffer for raw sensor data.
 *
 * @return true on success, false on timeout.
 */
static bool dht22_read_raw(uint8_t *data)
{
    /* Start signal: pull bus low for 2 ms. */
    gpio_set_direction(DHT22_GPIO_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT22_GPIO_NUM, 0);
    esp_rom_delay_us(2000);

    /* Release bus, wait 30 us, then switch to input. */
    gpio_set_level(DHT22_GPIO_NUM, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(DHT22_GPIO_NUM, GPIO_MODE_INPUT);

    /* Wait for DHT22 response: low ~80us then high ~80us. */
    if (!dht22_wait_level(0, DHT22_TIMEOUT_US)) {
        return false;
    }
    if (!dht22_wait_level(1, DHT22_TIMEOUT_US)) {
        return false;
    }
    if (!dht22_wait_level(0, DHT22_TIMEOUT_US)) {
        return false;
    }

    /* Read 40 bits. */
    for (int i = 0; i < 40; i++) {
        /* Each bit begins with ~50us low; wait for the rising edge. */
        if (!dht22_wait_level(1, DHT22_TIMEOUT_US)) {
            return false;
        }

        /* Sample after ~40us: high pulse = bit 1, low = bit 0. */
        esp_rom_delay_us(40);
        uint8_t bit = gpio_get_level(DHT22_GPIO_NUM);

        data[i / 8] <<= 1;
        data[i / 8] |= bit;

        /* Wait for the end of this bit (falling edge). */
        if (!dht22_wait_level(0, DHT22_TIMEOUT_US)) {
            return false;
        }
    }

    return true;
}

bool dht22_read(float *temperature, float *humidity)
{
    if (temperature == NULL || humidity == NULL) {
        return false;
    }

    uint8_t data[5] = {0};
    bool ok;

    portENTER_CRITICAL(&s_dht22_mux);
    ok = dht22_read_raw(data);
    portEXIT_CRITICAL(&s_dht22_mux);

    if (!ok) {
        return false;
    }

    /* Verify checksum: sum of first 4 bytes. */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return false;
    }

    /* Convert humidity (16-bit, 0.1% resolution). */
    uint16_t hum_raw = ((uint16_t)data[0] << 8) | data[1];
    *humidity = hum_raw * 0.1f;

    /* Convert temperature (16-bit signed, 0.1C resolution). */
    uint16_t temp_raw = ((uint16_t)data[2] << 8) | data[3];
    bool negative = (temp_raw & 0x8000) != 0;
    temp_raw &= 0x7FFF;
    *temperature = negative ? -(temp_raw * 0.1f) : (temp_raw * 0.1f);

    return true;
}
