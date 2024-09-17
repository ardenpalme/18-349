/**
 * @file kernel.c
 *
 * @brief   Kernel entry point
 *
 * @date    Mar 17th, 2021
 * @author  Arden Diakhate-Palme
 */

#include "arm.h"
#include "kernel.h"
#include "kmalloc.h"
#include "printk.h"
#include "uart_polling.h"
#include "uart.h"
#include "timer.h"
#include <led_driver.h>
#include <servok.h>
#include <i2c.h>

/** @brief - maximum UART buffer size */
#define MAX_BUF 512
/** @brief - UART transmit/receive buffer */
uint8_t uart_buf[MAX_BUF];

/** @brief - initializes peripherals and other control structures
 * then eneters user mode
 */
int kernel_main( void ) {
    init_349(); // DO NOT REMOVE THIS LINE
    i2c_master_init(0x50);
    led_driver_init();
	timer_start(SERVO_FREQ);
    uart_init(0);
    servo_init();
    enter_user_mode();
    return 0;
}
