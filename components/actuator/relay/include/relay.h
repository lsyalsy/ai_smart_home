#ifndef __RELAY_H
#define __RELAY_H

#include <stdbool.h>

/* 项目设计：加湿器继电器接 GPIO17 */
#define RELAY_GPIO 17

void relay_init(void);
void relay_on(void);
void relay_off(void);
void relay_toggle(void);
bool relay_state(void);

#endif /* __RELAY_H */
