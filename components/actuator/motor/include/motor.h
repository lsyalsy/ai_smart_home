#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdbool.h>

/* 项目设计：风扇电机接 GPIO16 */
#define MOTOR_GPIO 16

void motor_init(void);
void motor_on(void);
void motor_off(void);
void motor_toggle(void);
bool motor_state(void);

#endif /* __MOTOR_H */
