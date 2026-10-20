EE209 ATmega328PB SMT Firmware — board-specific port of uploaded code
=====================================================================

SOURCE
  Generated from the uploaded firmware files supplied on 21 October 2026.
  SMT pinout checked against SMTschem.pdf (14 October 2026).
  This is a separate project; your ATmega328P/Xplained Mini code is unchanged.

CHANGES
  - Firmware.cproj: target ATmega328PB (-mmcu=atmega328pb) in Debug and Release.
  - Firmware.componentinfo.xml: references updated to ATmega328PB.
  - ADC1 / PC1 => Vvf (voltage), ADC0 / PC0 => Vif (current).
  - ADC2 / PC2 => VvInt: boot reading DISABLED; SEM1 ADC2 field reports 0.
    This field is NOT a measurement of the analogue bias on the SMT board.
  - 74HC595 SH_CP=PB0, SH_DS=PB1, SH_ST=PB2, OE=PD3 active-low.
  - Four common-cathode digit controls unchanged: PD4-PD7 active-low.
  - UART TX = PD1 / USART0, 9600 baud 8N1; ISR uses USART0_UDRE_vect.
  - ESP32 receive is GPIO16, via the resistor divider already on the SMT PCB.
    Change ESP32 UART RX from GPIO15 to GPIO16 for the SMT configuration.
  - PB0 ADC scope-marker DISALLOWED because PB0 drives the display clock.
  - Sampling stays at requested 4800 TOTAL ADC triggers/sec, 240 V/I pairs.
  - The latest uploaded 50%-dim software and held-number 1-second scrolling
    behaviours are preserved; energy, maths, UART packet format unchanged.
  - Voltage/current gains are inherited starting estimates, NOT calibrated on SMT.
    CALIBRATION_COMPLETE remains 0.

BUILD IN MICROCHIP STUDIO
  1. Extract this folder into its own directory; keep the through-hole build.
  2. Open Firmware.cproj in Microchip Studio / Atmel Studio.
  3. Confirm selected MCU ATmega328PB, both configs -mmcu=atmega328pb.
  4. Verify the installed ATmega_DFP supports ATmega328PB; update Device Pack
     via Microchip Studio if it does not. If IDE prompts to update components,
     select the matching ATmega328PB device pack.
  5. Build > Rebuild Solution, prefer Release (-Os). Inspect *all* warnings.
  6. Check Program Memory and Data Memory usage; 2 KiB SRAM must also
     accommodate stack. Build .elf/.hex locally; none is included in this ZIP.

HARDWARE / PROGRAMMING
  - SMT PCB has an onboard 2 MHz external crystal (Y1, XTAL1/XTAL2).
    F_CPU=2 MHz is a compiler setting, NOT the fuse configuration.
    Confirm actual clock source and CKDIV8 before programming.
  - An ATmega328PB with erased/default fuses may run at an unexpected clock.
    Avoid blindly writing clock fuse bytes; use exact chip datasheet and TA.
  - Program via the 6-pin ISP header with compatible AVR ISP hardware:
    MISO, MOSI, SCK, RESET, Vtarget, GND (match the header's pin 1 marking).
  - Read signature before flashing: expected 0x1E9516.
  - Confirm correct power rails, polarities, and source isolation; previous
    resistor/regulator overheat or 5 V dropout must be resolved first.
  - Confirm UART and 1-second display rotation and the BLE/Wi-Fi API as needed.
  - Bench calibration is mandatory: reference Vrms, peak current, real power;
    check AVCC and analogue offset, no ADC clipping, nominal 500Hz input.

NOTES ON VERIFICATION
  - This package was checked for file consistency and for pin-map correctness.
  - It has NOT been executed on the SMT PCB or Proteus.
  - If a compatible AVR-GCC cross-compiler is not installed in the build
    environment, the target MCU compile must be done using Microchip Studio.
  - Do not copy the through-hole chip firmware ELF/HEX directly to ATmega328PB.
