/**
 * @file syscall.c
 *
 * @brief  System calls for kernel implementation
 *
 * @date   3/27/20
 *
 * @author Arden Diakhate-Palme
 */

#include <unistd.h>
#include <syscall.h>
#include <led_driver.h>
#include <i2c.h>
#include <nvic.h>
#include <printk.h>
#include <kernel.h>
#include "uart.h"

/** Standard out file I/O */
#define STDOUT 1

/** keeps track of top of heap */
char *heap_ptr;            

/**
 * @brief Extends the Heap by @param [incr] bytes.
 * @param [incr] Number of bytes to extend the heap by
 */
void *sys_sbrk(int incr){
    extern char __heap_low;
    extern char __heap_top;

    if(incr < 0) return (void*)-1; //check for heap overflow
	if(heap_ptr == NULL) heap_ptr= &__heap_low;
    if(heap_ptr + incr > &__heap_top)
        return (void*)-1;

    char *prev= heap_ptr;
    heap_ptr= (char*)heap_ptr + incr;
    return (void*)prev;
}

/**
 * @brief Writes chars from a string to a file descriptor
 * @param [file] fileNo to write to
 * @param [str]  buffered to write from
 * @param [len] length of string to write
 * @param [file] number of bytes to write from buffer
 */
int sys_write(int file, char *str, int len){
    if(file != STDOUT) return -1;
    int i=0;
    while(i < len){
        if(str[i] == '\0') break;
        while(uart_put_byte(str[i]));
        i++;
    }
    return len;
}

/**
 * @brief Reads char from file descriptor
 * @param [file] fileNo to write to
 * @param [str]  buffered to set bytes in
 * @param [len] length of string to write
 * @param [file] number of bytes to read into the buffer
 */
int sys_read(int file, char *str, int len){
    if(file) return -1;
    char c= 'a';
    int tmp;
    int i = 0;
	while(i < len){
		while((tmp = uart_get_byte(&c)) != 0);

        if(c == 0x4)
            return i;
        else if(c == '\b' && i > 0){
            str[--i]= 0;
            printk("\b \b");
        }else if(c == '\n' || c == '\r'){
            printk("\n");
            str[i] = c;
            i++;
            return i;
        }else{
            str[i] = c;
            uart_put_byte(c);
            i++;
        }
	}
    return i;
}

/**
 * @brief Reads char from file descriptor
 * @param [status] status no. with which to exit the program
 */
void sys_exit(int status){
   printk("Exited with status %d\n", status);
   uart_flush();
   led_set_display(status);
   
   //disable all interrupts and sleep permanently
   uint16_t i;
   for(i=0; i<256; i++){
        nvic_irq((uint8_t)i, IRQ_DISABLE);
   }

   while(1);
}
