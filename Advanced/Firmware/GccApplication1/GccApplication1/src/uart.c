/*
 * uart.c
 *
 * Created: 23/08/2026 10:06:01 am
 *  Author: talk2
 */ 
#include "uart.h"
#include <avr/io.h>

void uart_init(uint16_t ubrr)
{
    /*
     * Set the baud-rate register.
     *
     * For this task:
     *
     * F_CPU = 2 MHz
     * Baud  = 9600
     *
     * UBRR = F_CPU / (16 * Baud) - 1
     *
     *      = 2,000,000 / (16 * 9600) - 1
     *      = 2,000,000 / 153,600 - 1
     *      = 13.0208 - 1
     *      = 12.0208
     *
     * Therefore UBRR = 12.
     */

    // Upper part of UBRR.
    UBRR0H = (uint8_t)(ubrr >> 8);

    // Lower 8 bits of UBRR.
    UBRR0L = (uint8_t)ubrr;

    /*
     * Enable the transmitter.
     *
     * TXEN0 = 1
     */
    UCSR0B = (1 << TXEN0);

    /*
     * Configure USART for:
     *
     * Asynchronous mode
     * 8 data bits
     * No parity
     * 1 stop bit
     *
     * UCSZ01 = 1
     * UCSZ00 = 1
     */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}


void uart_transmit(uint8_t data)
{
    /*
     * Wait until the USART Data Register Empty flag is set.
     *
     * UDRE0 = 0 -> not ready
     * UDRE0 = 1 -> ready for another byte
     */
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    /*
     * Writing to UDR0 causes the USART hardware
     * to begin transmitting the byte.
     */
    UDR0 = data;
}


void uart_transmit_string(const char *string)
{
    /*
     * Continue until the null terminator '\0'
     * at the end of the string is reached.
     */
    while (*string != '\0')
    {
        uart_transmit(*string);
        string++;
    }
}