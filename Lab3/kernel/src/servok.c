/**
 * @file servok.c
 *
 * @brief  Control Servo Motors
 *
 * @date   03/30/2021
 *
 * @author  Neville Chima, Arden Diakhate-Palme
 */

#include <unistd.h>
#include <servok.h>
#include <gpio.h>
#include <syscall.h>
#include <printk.h>

/** indicates 20ms period for servo servo count at a rate of 0.1ms */
#define TOTAL_TICKS 200 

struct servo_ctrl servo_0;
struct servo_ctrl servo_1;

uint32_t get_pulse_width(uint8_t angle);

/** @brief - intialize Servo GPIO pins and servo control data structures
 */ 
void servo_init(){
  gpio_init(GPIO_B, 10, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT14); //Init Servo 0
  gpio_init(GPIO_A, 8, MODE_GP_OUTPUT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW, PUPD_NONE, ALT14); //Init Servo 1

  servo_0.en = 0;
  servo_0.state= 0;
  servo_0.ticks= 0;
  servo_0.on_count= 0;
  servo_0.off_count= 0;

  servo_1.en = 0;
  servo_1.state= 0;
  servo_1.ticks= 0;
  servo_1.on_count= 0;
  servo_1.off_count= 0;
}

/** @brief - Enable servo channels given user input
 */
int sys_servo_enable(uint8_t channel, uint8_t enabled){
    if(channel) servo_1.en = enabled;
    else servo_0.en = enabled;
    return 0;
}

/** @brief - Set servo channel to given agngle
 */
int sys_servo_set(uint8_t channel, uint8_t angle){
    if ((channel == 0 && !servo_0.en) || (channel == 1 && !servo_1.en))
        return -1;

    if(!channel) servo_0.ticks= get_pulse_width(angle);
    else servo_1.ticks= get_pulse_width(angle);
  return 0;
}

/** @brief - Formula used is 0.6ms + (angle * pulse_range)/max_angle
    where angle and max_angle are in degrees,
    max_angle = 180 degrees,
    pulse_range = 2.4ms - 0.6ms = 1.8ms,
    
    Math carried out in tenths of ms for suitable resolution
 */ 
uint32_t get_pulse_width(uint8_t angle){
  return 6 + angle/10;
}

/** @brief - sets servo GPIO pins to high if enabled, for a period corresponding to the 
 * requested angle. set GPIO pins to low if disabled
 */
void moveServo(struct servo_ctrl *servo, int channel){
    if(!servo->state){
        servo->on_count = 0;
        if(!channel) gpio_clr(GPIO_B, 10);
        else         gpio_clr(GPIO_A, 8);

        if(servo->off_count < TOTAL_TICKS - servo->ticks){
            servo->off_count++;
        }else servo->state = 1;

    }else{
        servo->off_count=0;
        if(!channel) gpio_set(GPIO_B, 10);
        else        gpio_set(GPIO_A, 8);

        if(servo->on_count < servo->ticks){
            servo->on_count++;
        }else servo->state = 0;
    }
}
