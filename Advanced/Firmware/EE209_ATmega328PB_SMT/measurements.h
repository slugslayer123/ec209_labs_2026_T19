#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H
#include <stdint.h>
#include "board_config.h"

typedef struct {
    uint32_t voltage_mv;
    uint32_t current_peak_ma;
    int32_t power_mw;
    uint32_t energy_mwh;
    uint16_t voltage_bias_adc;
    uint16_t current_bias_adc;
    uint16_t offset_adc_at_boot;
    uint8_t flags;
    /* flags: 1=calibrated; 2=complete; 4=sample loss since last window; 8=ADC clipping */
} meter_reading_t;

void measurements_init(uint16_t offset_adc);
void measurements_begin_window(void);
void measurements_set_elapsed_ms(uint32_t elapsed_ms);
void measurements_sample(uint8_t adc_channel, uint16_t raw_adc);
void measurements_mark_overrun(void);
uint8_t measurements_get_latest(meter_reading_t *out); /* reports each completed frame once */
const meter_reading_t *measurements_peek(void);
#endif
