#include "board_config.h"
#include "adc.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#if F_CPU != 2000000UL
#error Verify clock settings for acquisition
#endif

/* 250 pairs use 501 ADC samples (1002 bytes of SRAM). No per-sample math
 * or floating point occurs inside the interrupt. */
#define CAPTURE_COUNT ((uint16_t)(2u * MEASUREMENT_PAIRS + 1u))
static volatile uint16_t samples[CAPTURE_COUNT];
static volatile uint16_t captured;
static volatile uint8_t finished;
static uint8_t channel;
static uint16_t offset_at_boot;

void adc_init(void) {
#if ENABLE_ADC_SCOPE_MARKER
    DDRB |= _BV(PB0);
    PORTB &= (uint8_t)~_BV(PB0);
#endif
    offset_at_boot = 0xFFFFu;
#if ENABLE_BOOT_ADC2_READ
    ADMUX = _BV(REFS0) | ADC_OFFSET_CHANNEL;
    ADCSRB = 0;
    ADCSRA = _BV(ADEN) | _BV(ADPS2);
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) { }
    offset_at_boot = ADC;
#endif
    DIDR0 |= _BV(ADC0D) | _BV(ADC1D) | _BV(ADC2D);
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    OCR1A = (uint16_t)ADC_TIMER1_TOP;
    OCR1B = 8;
    captured = 0;
    finished = 0;
}

void adc_start_capture(void) {
    /* Call from foreground ONLY once previous capture was fully consumed. */
    TCCR1B = 0;
    captured = 0;
    finished = 0;
    channel = ADC_VOLTAGE_CHANNEL;
    ADMUX = _BV(REFS0) | ADC_VOLTAGE_CHANNEL;
    ADCSRB = _BV(ADTS2) | _BV(ADTS0); /* Timer1 COMPB */
    TCNT1 = 0;
    TIFR1 = _BV(OCF1B);
    /* ADIF is write-one-to-clear: discard any pending flag from ADC2. */
    ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADIF);
    ADCSRA |= _BV(ADSC);
    TCCR1B = _BV(WGM12) | _BV(CS10);
}

ISR(ADC_vect) {
#if ENABLE_ADC_SCOPE_MARKER
    PINB = _BV(PB0);
#endif
    const uint16_t raw = ADC;
    samples[captured] = raw;
    ++captured;
    if (captured >= CAPTURE_COUNT) {
        /* Stop capture; let foreground process at its own pace. */
        TCCR1B = 0;
        ADCSRA &= (uint8_t)~(_BV(ADATE) | _BV(ADIE));
        finished = 1;
        return;
    }
    channel = (channel == ADC_VOLTAGE_CHANNEL) ? ADC_CURRENT_CHANNEL : ADC_VOLTAGE_CHANNEL;
    ADMUX = _BV(REFS0) | channel;
    TIFR1 = _BV(OCF1B);
}

uint8_t adc_capture_ready(void) { return finished; }
uint16_t adc_captured_value(uint16_t index) {
    /* Valid only when adc_capture_ready() true and before start_capture(). */
    return index < CAPTURE_COUNT ? samples[index] : 0u;
}
uint16_t adc_boot_offset(void) { return offset_at_boot; }
