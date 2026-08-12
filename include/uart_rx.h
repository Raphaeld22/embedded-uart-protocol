#ifndef UART_RX_H
#define UART_RX_H

#include <stdbool.h>
#include <stdint.h>

#define UART_RX_BUFFER_SIZE 128u

void uart_rx_init(void);
void uart_rx_isr(uint8_t received_byte, uint32_t error_flags);
bool uart_rx_pop(uint8_t *byte);
bool uart_rx_overflowed(void);
void uart_rx_clear_overflow(void);

#endif
