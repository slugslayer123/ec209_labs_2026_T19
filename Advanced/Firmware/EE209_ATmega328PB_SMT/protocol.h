#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>
#include "measurements.h"
uint8_t protocol_encode(char *dst, uint8_t cap, const meter_reading_t *r, uint16_t seq);
#endif
