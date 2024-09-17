#ifndef _UART_POLLING_H_
#define _UART_POLLING_H_

/* @brief set USART_DIV=8.7 (see ref pg.523) */
#define BAUD_115200 0x8B

void uart_polling_init (int baud);
void uart_polling_put_byte (char c);
char uart_polling_get_byte ();

#endif /* _UART_POLLING_H_ */
