#include "board_config.h"
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile char tx_buf[UART_TX_BUFFER_SIZE];
static volatile uint8_t tx_head, tx_tail;
void uart_init(void) {
    tx_head = tx_tail = 0;
    UBRR0H = 0;
    UBRR0L = (uint8_t)((F_CPU + 8UL * UART_BAUD) / (16UL * UART_BAUD) - 1UL);
    UCSR0A = 0; /* normal speed; 2 MHz /16/13 = 9615.4 baud */
    UCSR0B = _BV(TXEN0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}
uint8_t uart_enqueue_bytes(const char *data, uint8_t len) {
    if (len >= UART_TX_BUFFER_SIZE) return 0;
    /* Main loop is the ONLY producer, ISR the ONLY consumer. Snapshotting the
     * 8-bit indices is atomic on AVR. The ISR cannot see bytes until we publish
     * tx_head at the end. Do NOT block interrupts while copying the packet;
     * otherwise the 6.25-kHz ADC service would miss deadlines. */
    const uint8_t start = tx_head;
    const uint8_t used = (uint8_t)((start - tx_tail) & (UART_TX_BUFFER_SIZE-1u));
    if ((uint8_t)(UART_TX_BUFFER_SIZE-1u-used) < len) return 0;
    uint8_t h = start;
    for (uint8_t n=0; n<len; ++n) {
        tx_buf[h] = data[n];
        h = (uint8_t)((h+1u)&(UART_TX_BUFFER_SIZE-1u));
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        tx_head = h;
        UCSR0B |= _BV(UDRIE0);
    }
    return 1;
}

ISR(USART_UDRE_vect) {
    if (tx_tail == tx_head) {
        UCSR0B &= (uint8_t)~_BV(UDRIE0);
    } else {
        UDR0 = tx_buf[tx_tail];
        tx_tail = (uint8_t)((tx_tail+1u)&(UART_TX_BUFFER_SIZE-1u));
    }
}
