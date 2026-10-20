#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* EE209 SMT PCB (SMTschem.pdf, 14 Oct 2026), onboard ATmega328PB.
 * Keep the known-good through-hole project separate from this build.
 * The board's 2 MHz external crystal requires appropriate clock fuses.
 */
#ifndef __AVR_ATmega328PB__
#error "Select ATmega328PB as the target device for this SMT firmware"
#endif
#ifndef F_CPU
#define F_CPU 2000000UL
#endif

#define ADC_VOLTAGE_CHANNEL      1u    /* SMT: PC1 / ADC1 = Vvf */
#define ADC_CURRENT_CHANNEL      0u    /* SMT: PC0 / ADC0 = Vif */
#define ADC_OFFSET_CHANNEL       2u    /* SMT: PC2 = VvInt, NOT 2.1VB (unused) */
#define ADC_REFERENCE_VOLTS      5.0f  /* AVCC reference nominally 5 V; verify physically */

/* Keep current tested settings: ~4808 total ADC conversions/s. */
#define ADC_TOTAL_HZ             4800UL
#define ADC_TIMER1_TOP           ((F_CPU / ADC_TOTAL_HZ) - 1UL)
#define MEASUREMENT_PAIRS        240u   

/* Starting gains copied from through-hole version: NOT SMT calibrated. */
#define VOLTS_PER_ADC_COUNT      0.106615f  /* provisional on SMT */
#define AMPS_PER_ADC_COUNT       0.004723f  /* provisional on SMT */
#define ANALOG_PHASE_CORR_DEG    0.0f    /* +ve advances voltage relative to I */
#define AC_FREQUENCY_HZ          500.0f
#define CALIBRATION_COMPLETE    0u      /* set 1 ONLY after reference measurements */

/* SMT: 74HC595 shift register uses PB0/PB1/PB2; OE is PD3. */
#define DISPLAY_SH_CP_PB         PB0
#define DISPLAY_SH_DS_PB         PB1
#define DISPLAY_SH_ST_PB         PB2
#define DISPLAY_OE_PD            PD3
#define DISPLAY_DIGIT_BASE_PD    PD4     /* PD4..PD7, active-low */

#define UART_BAUD               9600UL
#define UART_TX_BUFFER_SIZE     128u
#define ADC_RING_CAPACITY       128u      /* must be a power of 2 */

/* Diagnostics: read ADC2 once during initialization, before timer-triggered acquisition. */
#define ENABLE_BOOT_ADC2_READ   0u /* ADC2 = VvInt: do not report as bias */
#define ENABLE_ADC_SCOPE_MARKER 0u /* MUST remain 0: PB0 clocks the 74HC595 */
#if ENABLE_ADC_SCOPE_MARKER
#error "PB0 is the SMT display shift clock; scope marker is forbidden"
#endif
#endif
