#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* EE209 through-hole standard board with Xplained Mini, ATmega328P. */
#ifndef F_CPU
#define F_CPU 2000000UL
#endif

#define ADC_VOLTAGE_CHANNEL      0u    /* PC0: Vvf */
#define ADC_CURRENT_CHANNEL      1u    /* PC1: Vif */
#define ADC_OFFSET_CHANNEL       2u    /* PC2: buffered 2.1 V (check PCB) */
#define ADC_REFERENCE_VOLTS      5.0f  /* actual AVCC: MEASURE TOMORROW */

/* Timer1 period: 320 CPU cycles -> 6250 TOTAL conversions/s, 3125/ch. */
#define ADC_TOTAL_HZ             4800UL
#define ADC_TIMER1_TOP           ((F_CPU / ADC_TOTAL_HZ) - 1UL)
#define MEASUREMENT_PAIRS        240u   /* one result per 80 ms */

/* DO NOT CALL THESE CALIBRATED VALUES. Replace after scope/reference test. */
#define VOLTS_PER_ADC_COUNT      0.1000f  /* provisional, NOT verified on PCB */
#define AMPS_PER_ADC_COUNT       0.00475f /* provisional, NOT verified on PCB */
#define ANALOG_PHASE_CORR_DEG    0.0f    /* +ve advances voltage relative to I */
#define AC_FREQUENCY_HZ          500.0f
#define CALIBRATION_COMPLETE    0u      /* set 1 ONLY after reference measurements */

/* For standard PCB (not SMT). Shift-register segments assumed active-high. */
#define DISPLAY_SH_CP_PC         PC3
#define DISPLAY_SH_DS_PC         PC4
#define DISPLAY_SH_ST_PC         PC5
#define DISPLAY_OE_PB            PB1
#define DISPLAY_DIGIT_BASE_PD    PD4     /* PD4..PD7, digit active-low */

#define UART_BAUD               9600UL
#define UART_TX_BUFFER_SIZE     128u
#define ADC_RING_CAPACITY       128u      /* must be a power of 2 */

/* Diagnostics: read ADC2 once during initialization, before timer-triggered acquisition. */
#define ENABLE_BOOT_ADC2_READ   1u
#define ENABLE_ADC_SCOPE_MARKER 0u /* 1: toggle unused PB0 on each conversion (scope diagnostic) */
#endif
