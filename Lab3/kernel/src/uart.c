/**
 * @file uart.c
 *
 * @brief
 *
 * @date 03/19/2021
 *
 * @author  Arden Diakhate-Palme, Neville Chima
 */

#include <gpio.h>
#include <rcc.h>
#include <unistd.h>
#include <uart.h>
#include <uart_polling.h>
#include <nvic.h>
#include "printk.h"

/** @brief The UART register map. */
struct uart_reg_map {
    volatile uint32_t SR;   /**< Status Register */
    volatile uint32_t DR;   /**<  Data Register */
    volatile uint32_t BRR;  /**<  Baud Rate Register (i.e USART_DIV) */
    volatile uint32_t CR1;  /**<  Control Register 1 */
    volatile uint32_t CR2;  /**<  Control Register 2 */
    volatile uint32_t CR3;  /**<  Control Register 3 */
    volatile uint32_t GTPR; /**<  Guard Time and Prescaler Register */
};

/** @brief data structure to track kernel buffer
*/
struct uart_fifo {
    int write; /**< pointer to the next byt location to write  if the fifo is not full */
    int read; /**< pointer to the next byte location to read if the fifo is not empty */
    uint32_t count; /**< number of bytes in buffer */
};

/** @brief Base address for UART2 */
#define UART2_BASE  (struct uart_reg_map *) 0x40004400

/** @brief Enable  Bit for UART Config register */
#define UART_EN (1 << 13) 
/** @brief RX enable bit for UART Config register */
#define RX_EN (1 << 2) 
/** @brief TX enable bit for UART Config register */
#define TX_EN (1 << 3) 
/** @brief Enable Bit for UART clock in APB */
#define RCC_EN (1 << 17) 
/** @brief Transmission data reg empty */
#define TXE   (1 << 7) 
/** @brief TXE interrupt enable*/
#define TXEIE (1 << 7)
/** @brief TX complete and TXE set int enable*/ 
#define TCIE  (1 << 6) 
/** @brief TX complete and TXE set */
#define TC    (1 << 6) 
/** @brief UART serial line is idle RXNE set int_EN*/
#define IDLE   (1 << 4) 
/** @brief UART serial line is idle RXNE set*/
#define IDLEIE (1 << 4) 
 /** @brief Read data register not empty */
#define RXNE   (1 << 5)
/** @brief Read data register not empty interrupt enable */
#define RXNEIE (1 << 5) 

/** required USARTDIV to achieve 115200 Mhz freq */
#define USARTDIV 0x8B 

/** FIFO buffer length */
#define MAX_BUF 512 
/** max number of bytes to read/write */
#define MAX_COUNT 16 
/** external buffer to store transmitted bytes */
extern uint8_t uart_buf[MAX_BUF];

/** declares argument unused */
#define UNUSED __attribute__((unused)) 

/** global uart buffer FIFO control */
struct uart_fifo fifo;


void block_interrupts(struct uart_reg_map *uart);

/** @brief - initializes UART MMIO to USARTDIV baud rate and
 * enables UART interrupt
 */
void uart_init(UNUSED int baud){
    struct rcc_reg_map *rcc= RCC_BASE;
    struct uart_reg_map *uart = UART2_BASE;
    gpio_init(GPIO_A, 2, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW,
        PUPD_NONE, ALT7); //init TX (PA_2)
    gpio_init(GPIO_A, 3, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW,
        PUPD_NONE, ALT7); //init RX (PA_3)
    rcc->apb1_enr |= RCC_EN;                //enable APB peripheral clock
    uart->CR1 |= UART_EN | TX_EN | RX_EN;   //enable UART, TX, and RX
    uart->BRR |= USARTDIV;

    fifo.write = 0; //set buffer control to empty values
    fifo.read = 0;
    fifo.count = 0;

    nvic_irq(38, IRQ_ENABLE);   //enable USART2 IRQ

    return;
}

/** @brief - puts char value into next buffer location
 */
int uart_put_byte(char c){
    struct uart_reg_map *uart = UART2_BASE;
    block_interrupts(uart);

    int success = -1;
    if (fifo.count < MAX_BUF){
        uart_buf[fifo.write++] = c;
        fifo.count++;
        success = 0;
        if (fifo.write == MAX_BUF) {
            fifo.write = 0;
        }
    }

	uart->CR1 |= TXEIE;
	uart->CR1 &= ~RXNEIE;
    return success;
}

/** @brief - gets next char from buffer
 *  @param c - pointer to value where byte is store
 */
int uart_get_byte(char *c){
    struct uart_reg_map *uart = UART2_BASE;
    block_interrupts(uart);

    int success = -1;
    if (fifo.count > 0) {
        *c = uart_buf[fifo.read++];
        fifo.count--;
        success = 0;
        if (fifo.read == MAX_BUF){
            fifo.read = 0;
        }
    }

	//unblock receive interrupt
    uart->CR1&= ~TXEIE;
	uart->CR1|= RXNEIE; 
    return success;
}

/** @brief - services UART interrputs at the kernel level
 */
void uart_irq_handler(){
    nvic_clear_pending(38); //clear UART2 irq
    struct uart_reg_map *uart = UART2_BASE;
    int count, tmp;

    //Attempt to receive
    if((uart->SR & RXNE) && (uart->CR1 & RXNEIE)){
		if(fifo.count < MAX_BUF){
            uart_buf[fifo.write++] = uart->DR;
            fifo.count++;
            if (fifo.write == MAX_BUF){
                fifo.write = 0;
            }
		}
    }

    //Attempt transmit
    if((uart->SR & TXE) && (uart->CR1 & TXEIE)){
		uart->CR1 &= ~TXEIE; //mask TXE interrupt
        count = 0;
        while(count < MAX_COUNT && fifo.count > 0){
			tmp = uart->SR; //clear TXE bit
			(void)tmp;
            uart->DR = (uart->DR &  0xffffff00) | uart_buf[fifo.read++];
            fifo.count--;
            if (fifo.read == MAX_BUF){
                fifo.read = 0;
            }
            //Wait till byte transmits. Up to 16x times
            while(!(uart->SR & TXE));
            count++;
        }
    }
    return;
}

/** @brief - clears kernel buffer after user program using it finishes
 */
void uart_flush(){
    struct uart_reg_map *uart = UART2_BASE;
    char tmp;
    if(uart->SR & RXNE)
        tmp= uart->DR;
    while(!(uart->SR & TXE));

    int i=0;
    while(i<MAX_BUF)
        uart_buf[i++]= 0;
    (void)tmp;
}

/** @brief - blocks UART transmit/receive interrupts during critical section
 */
void block_interrupts(struct uart_reg_map *uart){
uart->CR1 &= ~TXEIE;
uart->CR1 &= ~RXNEIE;
}
