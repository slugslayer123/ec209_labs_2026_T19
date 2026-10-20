#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>
#include "measurements.h"
void display_init(void);
void display_refresh_one_digit(void);
void display_advance_mode(void);
void display_set_reading(const meter_reading_t *r);
#endif
