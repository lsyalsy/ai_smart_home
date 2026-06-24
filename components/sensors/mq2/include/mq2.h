#ifndef MQ2_H
#define MQ2_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize MQ-2 smoke sensor ADC (GPIO2 / ADC1_1).
 */
void mq2_init(void);

/**
 * @brief Read raw ADC value from MQ-2 sensor.
 *
 * @return Raw 12-bit ADC reading (0 ~ 4095).
 */
uint32_t mq2_read_raw(void);

/**
 * @brief Check if the MQ-2 reading exceeds the alarm threshold.
 *
 * @param threshold Raw ADC threshold value.
 * @return true if reading is greater than threshold, false otherwise.
 */
bool mq2_alarm(uint32_t threshold);

#ifdef __cplusplus
}
#endif

#endif /* MQ2_H */
