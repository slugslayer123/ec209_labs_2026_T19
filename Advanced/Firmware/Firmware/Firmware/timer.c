#include "board_config.h"
#include "timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile uint8_t display_due, second_due;
static volatile uint32_t uptime_ms;
void timer0_display_init(void) {
    /* 2,000,000/8/250 = 1000 ticks/s exactly. */
    TCCR0A = _BV(WGM01);
    TCCR0B = _BV(CS01);
    OCR0A = 249u;
    TCNT0 = 0;
    TIMSK0 |= _BV(OCIE0A);
}
ISR(TIMER0_COMPA_vect) {
    static uint16_t ms;
    ++ms;
    ++uptime_ms;
    if ((ms & 1u) == 0u) display_due = 1u;
    if (ms >= 1000u) { ms = 0; second_due = 1; }
}
uint8_t timer_take_display_due(void) {
    uint8_t value;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = display_due; display_due = 0; }
    return value;
}
uint8_t timer_take_one_second(void) {
    uint8_t value;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { value = second_due; second_due = 0; }
    return value;
}

uint32_t timer_millis(void) {
    uint32_t x;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { x = uptime_ms; }
    return x;
}
