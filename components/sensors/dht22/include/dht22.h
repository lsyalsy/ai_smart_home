#ifndef DHT22_H
#define DHT22_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize DHT22 single-wire interface.
 *
 * Configures GPIO3 as output with internal pull-up and releases the bus.
 */
void dht22_init(void);

/**
 * @brief Read temperature and humidity from DHT22.
 *
 * Performs start sequence, waits for sensor response, reads 40 bits,
 * verifies checksum and converts raw data to temperature/humidity.
 *
 * @param[out] temperature Temperature in Celsius.
 * @param[out] humidity    Relative humidity in percent.
 *
 * @return true on success, false on read timeout or checksum error.
 */
bool dht22_read(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif /* DHT22_H */
