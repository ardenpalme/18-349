#include <gpio.h>
#include <stdint.h>
#include <rcc.h>
#include <printk.h>
#include <unistd.h>
#include <adc.h>

struct adc_reg_map {
  volatile uint32_t SR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SMPR1;
  volatile uint32_t SMPR2;
  volatile uint32_t JOFR1;
  volatile uint32_t JOFR2;
  volatile uint32_t JOFR3;
  volatile uint32_t JOFR4;
  volatile uint32_t HTR;
  volatile uint32_t LTR;
  volatile uint32_t SQR1;
  volatile uint32_t SQR2;
  volatile uint32_t SQR3;
  volatile uint32_t JSQR;
  volatile uint32_t JDR1;
  volatile uint32_t JDR2;
  volatile uint32_t JDR3;
  volatile uint32_t JDR4;
  volatile uint32_t DR;
  volatile uint32_t CCR;
};

/** @brief base address for ADC */
#define ADC_BASE (struct adc_reg_map *) 0x40012000

/** @brief CR2 value to power on ADC */
#define ADON 1

/** @brief Enable value for ADC in RCC's APB reg */
#define ADC_EN (1 << 8)

/** @brief ADC clk prescaler for half frequency*/
#define ADCPRE ((1<<17) | (1<<16))

/** @brief Enable value for regular channel conversion */
#define SWSTART (1 << 30)

/** @brief Set single conversion mode */
#define CONT (1 << 1)

/** @brief Single conversion value */
#define SING_CONV 0xFF0FFFFF

/** @brief End of conversion*/
#define EOC (1<<4)

/** @brief initialize ADC hardware and GPIO pins */
void adc_init(){
  struct adc_reg_map *adc = ADC_BASE;
  struct rcc_reg_map *rcc = RCC_BASE;

  gpio_init(GPIO_B, 0, MODE_ANALOG_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW,
  PUPD_NONE,  ALT14); //Sensor init
  gpio_init(GPIO_C, 1, MODE_ANALOG_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW,
  PUPD_NONE, ALT14); //Michrophone init

  rcc->apb2_enr |= ADC_EN; //enable APB peripheral clock
  adc->CR2 |= ADON; //power ADC on

  adc->CCR&= ~ADCPRE; //prescale APB clk to set ADC clk

  adc->CR2|= ADON; //power ADC on
  return;
}

/** @brief read ADC conversion of a channel
 *  @param[in] specify conversion channel
 *  */
uint8_t adc_read_pin(uint8_t pin_num){
  struct adc_reg_map *adc = ADC_BASE;

  //specify regulat conversion mode: 1st conversion
  adc->SQR1&= SING_CONV;
  adc->SQR3= (adc->SQR3 & 0xFFFFFFE0) | pin_num;

  adc->CR2&= ~CONT; //set regular conversion mode
  adc->CR2|= SWSTART; //start regular conversion

  while (!(adc->SR & EOC)); //wait 'till EOC
  return adc->DR;
}
