#include "uart_rx.h"

#include <stddef.h>

static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile bool rx_overflow;

void uart_rx_init(void)
{
    rx_head = 0u;
    rx_tail = 0u;
    rx_overflow = false;
}

void uart_rx_isr(uint8_t received_byte, uint32_t error_flags)
{
    if (error_flags != 0u) {
        return;
    }

    uint8_t next = (uint8_t)((rx_head + 1u) % UART_RX_BUFFER_SIZE);

    if (next == rx_tail) {
        rx_overflow = true;
        return;
    }

    rx_buffer[rx_head] = received_byte;
    rx_head = next;
}

bool uart_rx_pop(uint8_t *byte)
{
    if (byte == NULL || rx_tail == rx_head) {
        return false;
    }

    *byte = rx_buffer[rx_tail];
    rx_tail = (uint8_t)((rx_tail + 1u) % UART_RX_BUFFER_SIZE);
    return true;
}

bool uart_rx_overflowed(void)
{
    return rx_overflow;
}

void uart_rx_clear_overflow(void)
{
    rx_overflow = false;
}
