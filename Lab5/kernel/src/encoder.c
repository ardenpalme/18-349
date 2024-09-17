#include <encoder.h>
#include <exti.h>
#include <gpio.h>
#include <unistd.h>
#include <nvic.h>
#include <printk.h>
#include <led_driver.h>
#include <arm.h>
#include <spi.h>

typedef enum {S00 = 0, S10 = 0x2, S11 = 0x3, S01 = 0x1} encoder_state;

/* 
 * IMPORTANT : 
 * Make sure these values are consistent with
 * the connections on your board!
 */
#define ENC0_A 7
#define ENC0_B 10
#define ENC0_IRQA 23
#define ENC0_IRQB 40

int curr_pos= 120;

/**
 * @brief Initialize the encoder
 * This only supports one encoder at a time
 */
void encoder_init() {
  gpio_init(GPIO_C, ENC0_A, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_HIGH, PUPD_NONE, ALT0);
  gpio_init(GPIO_A, ENC0_B, MODE_INPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_HIGH, PUPD_NONE, ALT0);

  enable_exti(GPIO_C, ENC0_A, RISING_FALLING_EDGE);
  enable_exti(GPIO_A, ENC0_B, RISING_FALLING_EDGE);

  nvic_irq(ENC0_IRQA, IRQ_ENABLE);
  nvic_irq(ENC0_IRQB, IRQ_ENABLE);

  return;
}

/**
 * @brief Stop the encoder
 * This only supports one encoder at a time
 */
void encoder_stop() {
  disable_exti(ENC0_A);
  disable_exti(ENC0_B);
  return;
}

/**
 * @brief Returns the current position of the encoder
 */
uint8_t encoder_read() {
    return curr_pos;
}

/**
 * @brief Handle the IRQ for the encoder
 * Calculate the position.
 */
encoder_state prev_state= S00;
void encoder_irq_handler() {
    exti_clear_pending_bit(GPIO_A);
    exti_clear_pending_bit(GPIO_C);

    int pinA= gpio_read(GPIO_C, 7);
    int pinB= gpio_read(GPIO_A, 10);

     encoder_state curr_state;
    if(!pinA){
        if(!pinB) curr_state= S00;
        else      curr_state= S01;
    }else{
        if(!pinB) curr_state= S10;
        else      curr_state= S11;
    }

    if(prev_state != curr_state){
        if(prev_state == S00){
            if(curr_state == S01) curr_pos++;
            else if(curr_state == S10) curr_pos--;

        }else if(prev_state == S01){
            if(curr_state == S11) curr_pos++;
            else if(curr_state == S00) curr_pos--;

        }else if(prev_state == S11){
            if(curr_state == S10) curr_pos++;
            else if(curr_state == S01) curr_pos--;

        }else if(prev_state == S10){
            if(curr_state == S00) curr_pos++;
            else if(curr_state == S11) curr_pos--;
        }
    }

    int fullRot= 240;
    if(curr_pos > fullRot) curr_pos= 240;
    if(curr_pos < 0)       curr_pos= 0;

    led_set_display(curr_pos);
    spi_slave_write(curr_pos);

    prev_state= curr_state; 
}
