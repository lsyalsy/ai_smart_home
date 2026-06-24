#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>
#include <stdbool.h>

/* 项目设计：蜂鸣器接 GPIO8 */
#define BUZZER_GPIO 8

void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);
void buzzer_toggle(void);
void buzzer_beep(uint32_t ms);
bool buzzer_state(void);

#endif /* __BUZZER_H */
