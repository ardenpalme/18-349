/**
 * @file kernel.c
 *
 * @brief      Kernel entry point
 *
 * @date    5/4/21
 * @author  Arden Diakhate-Palme
 */

#include "arm.h"
#include "kernel.h"
#include "printk.h"
#include "uart.h"
#include <led_driver.h>
#include <i2c.h>
#include "nvic.h"
#include "encoder.h"
#include "spi.h"
#include "motor_driver.h"
#include "gpio.h"

/** @brief - maximum UART buffer size */
#define MAX_BUF 512
/** @brief - UART transmit/receive buffer */
uint8_t uart_buf[MAX_BUF];


/** @brief - runs the kernel */
int kernel_main( void ) {
    i2c_master_init(0x50);
    led_driver_init(0);
    uart_init(0);
    encoder_init();
    spi_slave_init();
    motor_init(GPIO_A, GPIO_A, GPIO_A, 5, 6, 9, 1, 2, ALT1);

    while(1);

    return 0;
}
