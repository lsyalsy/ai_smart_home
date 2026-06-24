#ifndef __LED_H
#define __LED_H

#include <stdbool.h>

/* 项目设计：LED 灯光接 GPIO15 */
#define LED_GPIO 15

void led_init(void);
void led_on(void);
void led_off(void);
void led_toggle(void);
bool led_state(void);

#endif /* __LED_H */
