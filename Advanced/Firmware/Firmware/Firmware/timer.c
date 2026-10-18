
#include "board_config.h"
#include "timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#define DISPLAY_DIM_50_PERCENT 1u

static volatile uint8_t display_due, second_due;
static volatile uint32_t uptime_ms;

void timer0_display_init(void) {
	/* Timer0: 1 ms interrupt at 2 MHz CPU clock */
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

	if ((ms & 1u) == 0u) {
		display_due = 1u;
		} else {
		#if DISPLAY_DIM_50_PERCENT
		PORTD |= 0xF0u;  /* Turn off all four display digits */
		#endif
	}

	if (ms >= 1000u) {
		ms = 0;
		second_due = 1u;
	}
}

uint8_t timer_take_display_due(void) {
	uint8_t value;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		value = display_due;
		display_due = 0;
	}

	return value;
}

uint8_t timer_take_one_second(void) {
	uint8_t value;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		value = second_due;
		second_due = 0;
	}

	return value;
}

uint32_t timer_millis(void) {
	uint32_t value;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		value = uptime_ms;
	}

	return value;
}
