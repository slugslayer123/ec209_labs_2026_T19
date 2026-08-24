/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# "Insert system clock initialization code here" comment
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#define F_CPU 2000000UL

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "uart.h"


/*
 * Values required for the progress review.
 *
 * RMSVoltage  -> XX.X
 * PeakCurrent -> XXX
 * Power       -> X.XX
 */
#define RMSVoltage   14.5f
#define PeakCurrent  125
#define Power        1.60f


#define UART_UBRR    12


/*
 * Convert one numerical digit from 0-9 into
 * its ASCII character and transmit it.
 *
 * Example:
 *
 * digit = 5
 *
 * ASCII '0' = 48
 *
 * 5 + 48 = 53
 *
 * ASCII 53 = '5'
 */
static void transmit_digit(uint8_t digit)
{
    uart_transmit(digit + '0');
}


/*
 * Print RMS voltage in the required form:
 *
 * XX.X
 *
 * Example:
 * 14.5
 */
static void print_rms_voltage(float voltage)
{
    /*
     * Convert the floating-point value into tenths.
     *
     * Example:
     *
     * 14.5 * 10 = 145
     *
     * Adding 0.5 before conversion gives sensible
     * rounding for positive values.
     */
    uint16_t scaled_voltage =
        (uint16_t)(voltage * 10.0f + 0.5f);

    /*
     * For 145:
     *
     * Tens:
     * 145 / 100 = 1
     *
     * Units:
     * (145 / 10) % 10
     * = 14 % 10
     * = 4
     *
     * Decimal:
     * 145 % 10
     * = 5
     */

    uint8_t tens =
        scaled_voltage / 100;

    uint8_t units =
        (scaled_voltage / 10) % 10;

    uint8_t decimal =
        scaled_voltage % 10;

    transmit_digit(tens);
    transmit_digit(units);

    uart_transmit('.');

    transmit_digit(decimal);
}


/*
 * Print Peak Current in the required form:
 *
 * XXX
 *
 * Example:
 * 125
 */
static void print_peak_current(uint16_t current)
{
    /*
     * Hundreds:
     *
     * 125 / 100 = 1
     */
    uint8_t hundreds =
        current / 100;

    /*
     * Tens:
     *
     * (125 / 10) % 10
     * = 12 % 10
     * = 2
     */
    uint8_t tens =
        (current / 10) % 10;

    /*
     * Units:
     *
     * 125 % 10
     * = 5
     */
    uint8_t units =
        current % 10;

    transmit_digit(hundreds);
    transmit_digit(tens);
    transmit_digit(units);
}


/*
 * Print power in the required form:
 *
 * X.XX
 *
 * Example:
 * 1.60
 */
static void print_power(float power)
{
    /*
     * Convert the value into hundredths.
     *
     * Example:
     *
     * 1.60 * 100 = 160
     */
    uint16_t scaled_power =
        (uint16_t)(power * 100.0f + 0.5f);

    /*
     * For 160:
     *
     * Whole digit:
     * 160 / 100 = 1
     *
     * Tenths:
     * (160 / 10) % 10
     * = 16 % 10
     * = 6
     *
     * Hundredths:
     * 160 % 10
     * = 0
     */

    uint8_t whole =
        scaled_power / 100;

    uint8_t tenths =
        (scaled_power / 10) % 10;

    uint8_t hundredths =
        scaled_power % 10;

    transmit_digit(whole);

    uart_transmit('.');

    transmit_digit(tenths);
    transmit_digit(hundredths);
}


int main(void)
{
    /*
     * Initialise UART for 9600 baud.
     *
     * UBRR = 12 for:
     *
     * F_CPU = 2 MHz
     * Baud  = 9600
     */
    uart_init(UART_UBRR);

    while (1)
    {
        /*
         * Required first line:
         *
         * RMS Voltage is: XX.X
         */
        uart_transmit_string("RMS Voltage is: ");
        print_rms_voltage(RMSVoltage);

        uart_transmit('\r');
        uart_transmit('\n');


        /*
         * Required second line:
         *
         * Peak Current is: XXX
         */
        uart_transmit_string("Peak Current is: ");
        print_peak_current(PeakCurrent);

        uart_transmit('\r');
        uart_transmit('\n');


        /*
         * Required third line:
         *
         * Power is: X.XX
         */
        uart_transmit_string("Power is: ");
        print_power(Power);

        uart_transmit('\r');
        uart_transmit('\n');


        /*
         * Blank line between each group.
         */
        uart_transmit('\r');
        uart_transmit('\n');


        /*
         * Repeat once every second.
         */
        _delay_ms(1000);
    }
}
