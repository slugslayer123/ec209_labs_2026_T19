#include "board_config.h"
#include "adc.h"
#include "timer.h"
#include "uart.h"
#include "display.h"
#include "measurements.h"
#include "protocol.h"
#include <avr/interrupt.h>
#include <stdint.h>

#define CAPTURE_COUNT (2u * MEASUREMENT_PAIRS + 1u)

int main(void) {
    char packet[112];
    meter_reading_t reading;
    uint16_t sequence = 0;
    uint8_t tx_decimator = 0;
    uint32_t last_frame_ms = 0;

    uart_init();
    display_init();
    timer0_display_init();
    adc_init();
    measurements_init(adc_boot_offset());
    sei();
    adc_start_capture();

    for (;;) {
        if (adc_capture_ready()) {
            /* ADC/TIMER1 acquisition has stopped itself: input samples cannot
             * be overwritten while the foreground performs calculations. */
            const uint32_t now = timer_millis();
            uint32_t dt = last_frame_ms ? (now - last_frame_ms)
                : (uint32_t)(1000UL * (2UL * MEASUREMENT_PAIRS) / ADC_TOTAL_HZ);
            last_frame_ms = now;
            if (dt == 0u) dt = 1u;
            measurements_set_elapsed_ms(dt);
            measurements_begin_window();
            for (uint16_t j = 0; j < CAPTURE_COUNT; ++j) {
                const uint8_t ch = (j & 1u) ? ADC_CURRENT_CHANNEL : ADC_VOLTAGE_CHANNEL;
                measurements_sample(ch, adc_captured_value(j));
                /* Keep display refreshing even during foreground processing.
                 * The display routine now runs with global interrupts enabled,
                 * allowing the short ADC ISR to preempt shift-register writes. */
                if ((j & 15u) == 0u && timer_take_display_due()) {
                    display_refresh_one_digit();
                }
            }
            if (measurements_get_latest(&reading)) {
                display_set_reading(&reading);
                if (++tx_decimator >= 3u) {
                    tx_decimator = 0;
                    const uint8_t n = protocol_encode(packet, sizeof(packet), &reading, sequence++);
                    if (n) (void)uart_enqueue_bytes(packet, n);
                }
            }
            adc_start_capture();
        }
        /* Do not shift 74HC595 bits inside TIMER0_COMPA ISR: it blocks ADC ISR. */
        if (timer_take_display_due()) display_refresh_one_digit();
        if (timer_take_one_second()) display_advance_mode();
    }
}
