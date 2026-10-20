#include "protocol.h"
#include <stdio.h>
#include <string.h>
/* SEM1,seq,voltage_mV,current_peak_mA,power_mW,energy_mWh,Vbias,Ibias,ADC2boot,flags*XX\n */
uint8_t protocol_encode(char *dst, uint8_t cap, const meter_reading_t *r, uint16_t seq) {
    int n=snprintf(dst,cap,"SEM1,%u,%lu,%lu,%ld,%lu,%u,%u,%u,%u",
      (unsigned)seq,(unsigned long)r->voltage_mv,(unsigned long)r->current_peak_ma,
      (long)r->power_mw,(unsigned long)r->energy_mwh,
      (unsigned)r->voltage_bias_adc,(unsigned)r->current_bias_adc,
      (unsigned)r->offset_adc_at_boot,(unsigned)r->flags);
    if(n<0 || n+5 >= (int)cap) return 0;
    uint8_t crc=0;
    for (int i=0;i<n;i++) crc^=(uint8_t)dst[i];
    const char hex[]="0123456789ABCDEF";
    dst[n++]='*'; dst[n++]=hex[crc>>4]; dst[n++]=hex[crc&15]; dst[n++]='\r'; dst[n++]='\n';
    dst[n]='\0';
    return (uint8_t)n;
}
