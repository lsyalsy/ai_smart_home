#ifndef __LED_H
#define __LED_H

#include <stdbool.h>

/* 面包板方案：灯光通过继电器 IN3 (GPIO1) 控制 */
#define LED_GPIO 1

void led_init(void);
void led_on(void);
void led_off(void);
void led_toggle(void);
bool led_state(void);

#endif /* __LED_H */
