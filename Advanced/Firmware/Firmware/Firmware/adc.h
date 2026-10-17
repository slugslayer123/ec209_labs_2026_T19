#ifndef ADC_H
#define ADC_H
#include <stdint.h>
/* One full capture contains V,I,V,I,...,V (MEASUREMENT_PAIRS+1 V samples).
 * ADC capture stops automatically when the frame is full: NO streaming queue.
 */
void adc_init(void);
void adc_start_capture(void);
uint8_t adc_capture_ready(void);
uint16_t adc_captured_value(uint16_t index);
uint16_t adc_boot_offset(void);
#endif
