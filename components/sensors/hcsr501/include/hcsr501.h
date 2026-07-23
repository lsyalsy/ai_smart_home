#ifndef __HCSR501_H
#define __HCSR501_H

#include <stdbool.h>

/* 项目设计：HC-SR501 人体红外感应接 GPIO4 */
#define HCSR501_GPIO 5

void hcsr501_init(void);
bool hcsr501_detected(void);

#endif /* __HCSR501_H */
