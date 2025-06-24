#ifndef __UART_H
#define __UART_H


#include <stdint.h>


typedef void (*bl_uart_recv_cb_t)(uint8_t *data, uint32_t len);


void bl_uart_init(void);
void bl_uart_write(uint8_t *data, uint16_t len);
void bl_uart_recv_cb_register(bl_uart_recv_cb_t callback);


#endif