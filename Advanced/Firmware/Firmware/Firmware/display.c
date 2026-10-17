#include "display.h"
#include "board_config.h"
#include <avr/io.h>

#define SEG_DP 0x80u
#define SEG_BLANK 0x00u
#define SEG_P 0x73u
#define SEG_I 0x06u
#define SEG_DASH 0x40u

static const uint8_t segment_digits[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
static uint8_t patterns[4] = {SEG_DASH,SEG_DASH,SEG_DASH,SEG_DASH};
static uint8_t digit_pos, mode;
static meter_reading_t current;

static void shift8(uint8_t b) {
    PORTC &= (uint8_t)~_BV(DISPLAY_SH_CP_PC);
    PORTC &= (uint8_t)~_BV(DISPLAY_SH_ST_PC);
    for (uint8_t i=0; i<8; i++) {
        if (b & 0x80u) PORTC |= _BV(DISPLAY_SH_DS_PC);
        else PORTC &= (uint8_t)~_BV(DISPLAY_SH_DS_PC);
        PORTC |= _BV(DISPLAY_SH_CP_PC);
        PORTC &= (uint8_t)~_BV(DISPLAY_SH_CP_PC);
        b <<= 1;
    }
    PORTC |= _BV(DISPLAY_SH_ST_PC);
    PORTC &= (uint8_t)~_BV(DISPLAY_SH_ST_PC);
}
static void show_tenths(uint16_t n, uint8_t prefix) {
    /* U? No room for U+14.0; voltage is displayed as 14.0 (with blank left). */
    uint8_t d0, d1, d2, d3;
    if (n>9999u) { for (uint8_t k=0;k<4;k++) patterns[k]=SEG_DASH; return; }
    d0=(uint8_t)((n/1000u)%10u);
    d1=(uint8_t)((n/100u)%10u);
    d2=(uint8_t)((n/10u)%10u);
    d3=(uint8_t)(n%10u);
    if (prefix) { /* P7.5 for expected range 0..9.9 W */
        patterns[0]=SEG_P;
        patterns[1]=SEG_BLANK;
        patterns[2]=segment_digits[d2] | SEG_DP;
        patterns[3]=segment_digits[d3];
    } else {
        patterns[0] = d0 ? segment_digits[d0] : SEG_BLANK;
        patterns[1] = (d0||d1) ? segment_digits[d1] : SEG_BLANK;
        patterns[2] = segment_digits[d2] | SEG_DP;
        patterns[3] = segment_digits[d3];
    }
}
static void rebuild(void) {
    if (!(current.flags & 2u)) { for(uint8_t k=0;k<4;k++) patterns[k]=SEG_DASH; return; }
    if (mode==0) { /* Voltage numeric with 0.1-V resolution. */
        uint32_t x=(current.voltage_mv+50u)/100u;
        show_tenths((uint16_t)(x>9999u?10000u:x),0);
    } else if (mode==1) { /* Peak current in mA, four-digit display */
    uint32_t i = current.current_peak_ma;

    if (i > 9999u) {
	    for (uint8_t k=0; k<4; k++) patterns[k]=SEG_DASH;
	    return;
    }

    patterns[0] = (i >= 1000u) ? segment_digits[(i/1000u)%10u] : SEG_BLANK;
    patterns[1] = (i >= 100u)  ? segment_digits[(i/100u)%10u]  : SEG_BLANK;
    patterns[2] = (i >= 10u)   ? segment_digits[(i/10u)%10u]   : SEG_BLANK;
    patterns[3] = segment_digits[i%10u];
    } else { /* Real power in W, numeric display */
    const int32_t mw = current.power_mw;

    if (mw < 0 || mw >= 999950L) {
	    for (uint8_t k = 0; k < 4; k++)
	    patterns[k] = SEG_DASH;
	    return;
    }

    show_tenths((uint16_t)((mw + 50L) / 100L), 0);
    }
}

void display_init(void) {
    DDRC |= _BV(DISPLAY_SH_CP_PC)|_BV(DISPLAY_SH_DS_PC)|_BV(DISPLAY_SH_ST_PC);
    DDRB |= _BV(DISPLAY_OE_PB);
    DDRD |= 0xF0u;
    PORTD |= 0xF0u; /* all digits OFF (common cathodes active-low) */
    PORTB &= (uint8_t)~_BV(DISPLAY_OE_PB); /* 74HC595 OE active-low */
    digit_pos=0; mode=0;
    shift8(0);
}
void display_refresh_one_digit(void) {
    PORTD |= 0xF0u;  /* blank while changing segments: avoids ghosting */
    shift8(patterns[digit_pos]);
    PORTD &= (uint8_t)~(1u<<(DISPLAY_DIGIT_BASE_PD+digit_pos));
    digit_pos=(uint8_t)((digit_pos+1u)&3u);
}
void display_advance_mode(void) { mode=(uint8_t)((mode+1u)%3u); rebuild(); }
void display_set_reading(const meter_reading_t *r) { current=*r; rebuild(); }
