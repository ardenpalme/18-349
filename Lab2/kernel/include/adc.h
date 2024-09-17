// James Zhang
#ifndef _ADC_H_
#define _ADC_H_

#include <unistd.h>

void adc_init();
uint8_t adc_read_pin(uint8_t pin_num);

#endif /* _ADC_H_ */
