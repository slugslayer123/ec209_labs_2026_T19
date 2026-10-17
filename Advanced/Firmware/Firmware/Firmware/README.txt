EE209 bounded-frame capture candidate (NOT yet verified in Proteus / hardware)

RATIONALE:
  Previous queue design overflowed, and the 32-bit accumulator-in-ISR patch
  displayed ---- with no serial output. Here, ADC ISR *only* stores raw samples.
  After exactly 2*MEASUREMENT_PAIRS+1 samples, ADC capture stops. Foreground
  processes the samples, updates display and UART, then starts another capture.
  This prevents dropped raw samples due to foreground processing delays.

INSTALL:
  1. BACK UP currently working Microchip Studio project.
  2. Restore board_config.h, uart.c/h, protocol.c/h, display.c/h from
     original EE209_Complete_Firmware archive if modified for diagnostics.
  3. Replace adc.c/h, main.c, measurements.c/h, timer.c/h with this folder's
     eight files. Keep existing board_config.h, uart, protocol, display files.
  4. board_config.h: ADC_TOTAL_HZ=6250UL; ENABLE_ADC_SCOPE_MARKER=1u;
     CALIBRATION_COMPLETE=0u. Use compiler -Os and correct 2 MHz MCU clock.
  5. Clean/rebuild in Microchip Studio. Check RAM data usage and warnings.
  6. In Proteus: ideal generators Vs 0.968 Vpeak and Is 0.520 Vpeak,
     2.5 V offset, 500 Hz, phase 0, both connected to ADC0 and ADC1.
  7. Confirm SEM1 reports near 14V, 505mA peak, 5W; display scrolls.
  8. Try phase +90, -90, then 53.13 degrees, and check energy accumulation.

CAUTIONS:
  - This is a candidate workaround, not yet verified by an AVR toolchain or
    Proteus execution. DO NOT present as fully tested or calibrated.
  - It samples in finite frames, pausing the ADC during processing and
    formatting. PB0 will therefore toggle during each acquisition burst and
    remain constant between captures. THIS IS INTENTIONAL.
  - Energy uses elapsed milliseconds between measurement frames to account
    for capture gaps, assuming that the most recently measured power is a
    reasonable estimate during the processing interval.
  - SRAM cost of sample buffer is 2*(2*MEASUREMENT_PAIRS+1) bytes = 1002
    for MEASUREMENT_PAIRS=250. Check build SRAM utilization on 2KB ATmega328P.
  - Moving display multiplexing into the Timer0 ISR is intended to keep it
    refreshed during foreground math. Verify no ghosting or timing problems.
  - If any issues, revert to backed-up baseline firmware and report results.
  - When physically testing the standard PCB, verify the MCU, clock, ADC
    voltage levels, channel mapping, UART voltage divider, and calibration.
