

#include "board_config.h"
#include "adc.h"
#include "timer.h"
#include "uart.h"
#include "display.h"
#include "measurements.h"
#include "protocol.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <stdint.h>

#define CAPTURE_COUNT (2u * MEASUREMENT_PAIRS + 1u)

/*
 * Internal 8 MHz RC oscillator / 4 = approximately 2 MHz.
 *
 * Requires clock-source fuses to select the internal 8 MHz
 * oscillator. Does not modify clock-source fuses.
 */
static void clock_init(void) {
    clock_prescale_set(clock_div_4);
}

int main(void) {
    char packet[112];
    meter_reading_t reading;

    uint16_t sequence = 0;
    uint8_t tx_decimator = 0;
    uint8_t display_initialized = 0;
    uint32_t last_frame_ms = 0;

    /* Set CPU clock before initialising any peripherals. */
    clock_init();

    uart_init();
    display_init();
    timer0_display_init();
    adc_init();
    measurements_init(adc_boot_offset());

    sei();
    adc_start_capture();

    for (;;) {
        if (adc_capture_ready()) {
            const uint32_t now = timer_millis();

            uint32_t dt = last_frame_ms
                ? (now - last_frame_ms)
                : (uint32_t)(
                    1000UL * (2UL * MEASUREMENT_PAIRS)
                    / ADC_TOTAL_HZ
                );

            last_frame_ms = now;

            if (dt == 0u) {
                dt = 1u;
            }

            measurements_set_elapsed_ms(dt);
            measurements_begin_window();

            for (uint16_t j = 0; j < CAPTURE_COUNT; ++j) {
                const uint8_t ch = (j & 1u)
                    ? ADC_CURRENT_CHANNEL
                    : ADC_VOLTAGE_CHANNEL;

                measurements_sample(
                    ch,
                    adc_captured_value(j)
                );

                if ((j & 15u) == 0u &&
                    timer_take_display_due()) {
                    display_refresh_one_digit();
                }
            }

            if (measurements_get_latest(&reading)) {

                /* Latch the first display reading at startup. */
                if (!display_initialized) {
                    display_set_reading(&reading);
                    display_initialized = 1u;
                }

                /* Send each third measurement frame over UART. */
                if (++tx_decimator >= 3u) {
                    tx_decimator = 0u;

                    const uint8_t n = protocol_encode(
                        packet,
                        sizeof(packet),
                        &reading,
                        sequence++
                    );

                    if (n) {
                        (void)uart_enqueue_bytes(packet, n);
                    }
                }
            }

            adc_start_capture();
        }

        /* Refresh one physical display digit. */
        if (timer_take_display_due()) {
            display_refresh_one_digit();
        }

        /* Change measurement and latch a stable reading. */
        if (timer_take_one_second()) {
            display_advance_mode();

            if (display_initialized) {
                display_set_reading(&reading);
            }
        }
    }
}

