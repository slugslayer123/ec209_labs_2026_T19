#ifndef UART_H
#define UART_H
#include <stdint.h>
void uart_init(void);
uint8_t uart_enqueue_bytes(const char *message, uint8_t length);
#endif
