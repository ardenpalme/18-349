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
#include "printk.h"
#include "uart.h"
#include "timer.h"
#include <led_driver.h>
#include <i2c.h>
#include <mpu.h>

/** @brief - maximum UART buffer size */
#define MAX_BUF 512
/** @brief - UART transmit/receive buffer */
uint8_t uart_buf[MAX_BUF];
/** @brief - MPU regions setup kernel and user side*/
void protect_memory();

/** @brief - initializes peripherals and other control structures
 * then eneters user mode
 */
int kernel_main( void ) {
    init_349(); // DO NOT REMOVE THIS LINE
    i2c_master_init(0x50);
    led_driver_init(0);
    uart_init(0);
    protect_memory();
    enter_user_mode();
    return 0;
}

void protect_memory(){
    int status;

    //16KB of user code
    extern char _swi_stub_start;
    if( (status= mm_region_enable(0, &_swi_stub_start, mm_log2ceil_size(16384), 1, 0)) < 0)
        breakpoint();

    //2KB of read-only data
    extern char _u_rodata;
    if( (status= mm_region_enable(1, &_u_rodata, mm_log2ceil_size(2048), 1, 0)) < 0)
        breakpoint();

    //1KB of user data
    extern char _u_data;
    if( (status= mm_region_enable(2, &_u_data, mm_log2ceil_size(1024), 0, 1)) < 0)
        breakpoint();

    //1KB of user BSS
    extern char _u_bss;
    if( (status= mm_region_enable(3, &_u_bss, mm_log2ceil_size(1024), 0, 1)) < 0)
        breakpoint();

    //4KB of user heap
    extern char __heap_low;
    if( (status= mm_region_enable(4, &__heap_low, mm_log2ceil_size(4096), 0, 1)) < 0)
        breakpoint();
        
    //2KB of default thread stack space 
    extern char __psp_stack_bottom;
    if( (status= mm_region_enable(5, &__psp_stack_bottom, mm_log2ceil_size(2048), 0, 1)) < 0)
        breakpoint();
}
