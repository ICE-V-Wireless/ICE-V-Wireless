/*
 * adc_c3.h - ADC driver for ESP32C3
 * 05-18-22 E. Brombaugh
 */

#ifndef __ADC_C3__
#define __ADC_C3__

#include "main.h"

esp_err_t adc_c3_init(void);
int32_t adc_c3_get(void);

#endif
