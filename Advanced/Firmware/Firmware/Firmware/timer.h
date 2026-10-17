#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
void timer0_display_init(void);
uint32_t timer_millis(void);
uint8_t timer_take_display_due(void);
uint8_t timer_take_one_second(void);
#endif
