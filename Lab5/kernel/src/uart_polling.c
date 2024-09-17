#include <gpio.h>
#include <rcc.h>
#include <unistd.h>
#include <uart_polling.h>

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

/** @brief Base address for UART2 */
#define UART2_BASE  (struct uart_reg_map *) 0x40004400

/** @brief Enable  Bit for UART Config register */
#define UART_EN (1 << 13)

/** @brief TX enable bit for UART Config register */
#define RX_EN (1 << 2)

/** @brief TX enable bit for UART Config register */
#define TX_EN (1 << 3)

/** @brief Enable Bit for UART clock in APB*/
#define RCC_EN (1 << 17)

/** @brief Transmission data reg empty */
#define TXE (1 << 7)

/** @brief Read data register not empty */
#define RXNE (1 << 5)

/** @brief required USARTDIV to achieve 115200 Mhz freq */
#define USARTDIV 0x8B
/**
 * @brief initializes UART to given baud rate with 8-bit word length, 1 stop bit, 0 parity bits
 *
 * @param[in] baud Baud rate
 */
void uart_polling_init (int baud){
    (void) baud;

    struct rcc_reg_map *rcc= RCC_BASE;
    struct uart_reg_map *uart = UART2_BASE;

    gpio_init(GPIO_A, 2, MODE_ALT, OUTPUT_PUSH_PULL, OUTPUT_SPEED_LOW,
        PUPD_NONE, ALT7); //init TX (PA_2)

    gpio_init(GPIO_A, 3, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW,
        PUPD_NONE, ALT7); //init RX (PA_3)

    rcc->apb1_enr |= RCC_EN;                //enable APB peripheral clock
    uart->CR1 |= UART_EN | TX_EN | RX_EN;   //enable UART, TX, and RX
    uart->BRR |= USARTDIV;
    return;
}

/**
 * @brief transmits a byte over UART
 *
 * @param[in] c character to be sent
 */
void uart_polling_put_byte (char c){
    struct uart_reg_map *uart= UART2_BASE;
    while(!(uart->SR & TXE)); //if CTS from hardware, then send
    uart->DR= c;
    return;
}

/**
 * @brief receives a byte over UART
 */
char uart_polling_get_byte (){
    struct uart_reg_map *uart= UART2_BASE;
    while(!(uart->SR & RXNE)); //if read data reg not empty, read
    return (char)uart->DR;
}
