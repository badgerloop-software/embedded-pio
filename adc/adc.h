#ifndef __ADC_H__
#define __ADC_H__

#include <zephyr/kernel.h>
#include <soc.h>

#define ADC_MAX_VALUE 4096 // because we use 12 bit ADC

void initADC(ADC_TypeDef* instance);
float readADC(uint32_t channel);

#endif