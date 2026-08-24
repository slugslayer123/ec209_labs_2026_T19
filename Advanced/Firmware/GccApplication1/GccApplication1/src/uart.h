/*
 * uart.h
 *
 * Created: 23/08/2026 10:05:23 am
 *  Author: talk2
 */ 


#ifndef UART_H_
#define UART_H_

#include <stdint.h>

/*
 * Initialise USART0 using the supplied UBRR value.
 */
void uart_init(uint16_t ubrr);

/*
 * Transmit one byte through USART0.
 */
void uart_transmit(uint8_t data);

/*
 * Transmit a null-terminated string through USART0.
 */
void uart_transmit_string(const char *string);

#endif /* UART_H_ */