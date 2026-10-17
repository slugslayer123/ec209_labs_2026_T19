#include "measurements.h"
#include <math.h>
#include <string.h>

/* All running statistics are evaluated in the main loop, NOT the ADC ISR.
 * A pair is completed when the next voltage sample arrives:
 *  voltage(t0), current(t0+160us), voltage(t0+320us).
 * Sum/difference provide quadrature reconstruction for nominal 500Hz AC.
 */
#define PI_F 3.14159265358979323846f

typedef struct {
    uint32_t sum_v, sum_i, sum_v2, sum_i2;
    int32_t sum_vm_i, sum_vd_i; /* (V0+V1)*I, (V1-V0)*I */
    int32_t sum_vm, sum_vd;     /* V0+V1, V1-V0 */
    uint16_t i_min, i_max;
    uint16_t v_min, v_max;
    uint16_t n;
} accumulation_t;

static accumulation_t a;
static uint16_t previous_v, pending_i;
static uint8_t have_v, have_i;
static uint8_t window_fault;
static meter_reading_t latest;
static uint8_t pending_result;
static float energy_wh;
static uint32_t elapsed_window_ms;
static float center_coefficient, quadrature_coefficient;

static uint32_t to_u32(float f) { return f <= 0 ? 0 : (f >= 4294967040.0f ? 4294967295UL : (uint32_t)(f + .5f)); }
static int32_t to_i32(float f) { return f > 2147483000.f ? 2147483000L : f < -2147483000.f ? -2147483000L : (int32_t)(f + (f >= 0 ? .5f : -.5f)); }

static void reset_window(void) {
    memset(&a, 0, sizeof(a));
    a.i_min = 1023u;
    a.v_min = 1023u;
    have_v = 0;
    have_i = 0;
}

void measurements_init(uint16_t offset_adc) {
    memset(&latest, 0, sizeof(latest));
    latest.offset_adc_at_boot = offset_adc;
    energy_wh = 0;
    elapsed_window_ms = (1000UL * (2UL * MEASUREMENT_PAIRS)) / ADC_TOTAL_HZ;
    /* Precompute slow trig in setup (2 MHz AVR): no trig in live window path. */
    const float delta_s = 1.0f / (float)ADC_TOTAL_HZ;
    const float theta = 2.0f * PI_F * AC_FREQUENCY_HZ * delta_s;
    const float phase = ANALOG_PHASE_CORR_DEG * (PI_F/180.0f);
    center_coefficient = cosf(phase) / cosf(theta);
    quadrature_coefficient = sinf(phase) / sinf(theta);
    pending_result = 0;
    window_fault = 0;
    reset_window();
}

void measurements_begin_window(void) {
    reset_window();
}

void measurements_set_elapsed_ms(uint32_t ms) {
    /* Energy is integrated over actual wall time, including processing gaps. */
    elapsed_window_ms = ms;
}

void measurements_mark_overrun(void) {
    window_fault = 1u;
    reset_window();
}

static void publish_window(void) {
    const float n = (float)a.n;
    const float vm = (float)a.sum_vm / (2.0f*n);
    const float im = (float)a.sum_i / n;
    float vrms_count2 = (float)a.sum_v2 / n - ((float)a.sum_v / n)*((float)a.sum_v/n);
    /* V statistics use V-next exactly once per power pair. */
    float p_center = (float)a.sum_vm_i / (2.0f*n) - vm*im;
    float p_quadrature = (float)a.sum_vd_i / (2.0f*n) - ((float)a.sum_vd/(2.0f*n))*im;
    float power_counts = p_center*center_coefficient + p_quadrature*quadrature_coefficient;
    const float volts = sqrtf(vrms_count2 > 0 ? vrms_count2 : 0)*VOLTS_PER_ADC_COUNT;
    const float max_abs_i = fmaxf(fabsf((float)a.i_min-im), fabsf((float)a.i_max-im));
    const float amps_peak = max_abs_i*AMPS_PER_ADC_COUNT;
    const float watts = power_counts*VOLTS_PER_ADC_COUNT*AMPS_PER_ADC_COUNT;

    latest.voltage_mv = to_u32(volts*1000.0f);
    latest.current_peak_ma = to_u32(amps_peak*1000.0f);
    latest.power_mw = to_i32(watts*1000.0f);
    latest.voltage_bias_adc = (uint16_t)((a.sum_v + (a.n/2))/a.n);
    latest.current_bias_adc = (uint16_t)((a.sum_i + (a.n/2))/a.n);
    latest.flags = (CALIBRATION_COMPLETE ? 1u : 0u) | 2u | (window_fault ? 4u : 0u);
    if (a.i_min <= 2u || a.i_max >= 1021u || a.v_min <= 2u || a.v_max >= 1021u) latest.flags |= 8u;
    /* 250 pairs at 3125 pairs/sec: 80ms. Mark lost windows instead of hiding faults. */
    energy_wh += watts * ((float)elapsed_window_ms / 3600000.0f);
    if (energy_wh < 0) energy_wh = 0;
    latest.energy_mwh = to_u32(energy_wh*1000.0f);
    pending_result = 1;
    window_fault = 0;
    const uint16_t boundary_voltage = previous_v;
    reset_window();
    previous_v = boundary_voltage;
    have_v = 1; /* reuse the boundary sample; no missing pair at frame edges */
}

void measurements_sample(uint8_t ch, uint16_t raw) {
    if (ch == ADC_VOLTAGE_CHANNEL) {
        if (!have_v) { previous_v=raw; have_v=1; return; }
        if (!have_i) { previous_v=raw; return; }
        /* Integer running sums fit int32 for <= 250 samples and 10-bit ADC. */
        const int32_t vsum = (int32_t)previous_v + raw;
        const int32_t vdiff = (int32_t)raw - previous_v;
        a.sum_v += raw;
        a.sum_i += pending_i;
        a.sum_v2 += (uint32_t)raw*raw;
        a.sum_i2 += (uint32_t)pending_i*pending_i;
        a.sum_vm_i += vsum*(int32_t)pending_i;
        a.sum_vd_i += vdiff*(int32_t)pending_i;
        a.sum_vm += vsum;
        a.sum_vd += vdiff;
        if (raw < a.v_min) a.v_min=raw;
        if (raw > a.v_max) a.v_max=raw;
        if (pending_i < a.i_min) a.i_min=pending_i;
        if (pending_i > a.i_max) a.i_max=pending_i;
        ++a.n;
        previous_v = raw;
        have_i = 0;
        if (a.n == MEASUREMENT_PAIRS) publish_window();
    } else if (ch == ADC_CURRENT_CHANNEL) {
        if (have_v) { pending_i=raw; have_i=1; }
    }
}

uint8_t measurements_get_latest(meter_reading_t *out) {
    if (!pending_result) return 0;
    *out = latest;
    pending_result = 0;
    return 1;
}
const meter_reading_t *measurements_peek(void) { return &latest; }
