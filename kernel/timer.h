#ifndef MATENOS_TIMER_H
#define MATENOS_TIMER_H

#include <stdint.h>

void timer_init(void);
void timer_tick(void);
uint32_t timer_ticks(void);

#endif
