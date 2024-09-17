/**
 * @file   timer.c
 *
 * @brief  configures the systick timer
 *
 * @date   Mar 17, 2021
 *
 * @author Arden Diakhate-Palme, Neville Chima
 */

#include <timer.h>
#include <unistd.h>
#include <printk.h>
#include <servok.h>
#include <gpio.h>

/** @brief specifies a structure to access systick register map */
struct STK_reg_map{
    volatile uint32_t CTRL;  /**< Control reg */
    volatile uint32_t LOAD;  /**< RELOAD value reg*/
    volatile uint32_t VAL;   /**< current value reg */
    volatile uint32_t CALIB; /**< calibration reg */
};
/** @brief base address of systick regmap*/
#define STK_BASE (struct STK_reg_map *)0xE000E010

/** @brief Enables the systick interupt */
#define TICKINT   (1 << 1)

/** @brief Enables systick in control reg*/
#define ENABLE    1

/** @brief Enable higher speed clock */
#define CLKSOURCE (1 << 2)

/** @brief unused input func parameter specifier */
#define UNUSED __attribute__((unused))

/** @brief servo_0 PWM generation structure*/
extern struct servo_ctrl servo_0;

/** @brief servo_1 PWM generation structure*/
extern struct servo_ctrl servo_1;

/** @brief starts systick clock at specified frequency */
int timer_start(UNUSED int frequency){
    struct STK_reg_map *stk= STK_BASE;

    stk->LOAD= (stk->LOAD & 0xff000000) | frequency;
    stk->VAL= 0;

    //enable the interupt once ct==0 and start timer
    stk->CTRL|= TICKINT;
    stk->CTRL|= ENABLE;
    stk->CTRL|= CLKSOURCE;

    return 0;
}

/** @brief disables the systick timer */
void timer_stop(){
    struct STK_reg_map *stk= STK_BASE;
    stk->CTRL&= ~ENABLE;
}

/** @brief generates the PWM signals for both servo motors.
 * This runs every 0.1ms.
 */
void systick_c_handler(){
    if(servo_0.en) moveServo(&servo_0, 0);
    if(servo_1.en) moveServo(&servo_1, 1);
}
