#include "protocol.h"
#include "uart_rx.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define LOCAL_ADDRESS 0x02u
#define CMD_SET_VALUE 0x10u

static bool frame_is_for_local_device(const Frame *frame)
{
    return frame->destination == LOCAL_ADDRESS ||
           frame->destination == FRAME_BROADCAST_ADDRESS;
}

static bool frame_content_is_valid(const Frame *frame)
{
    if (frame->command == CMD_SET_VALUE) {
        return frame->length == 2u;
    }

    return false;
}

static void process_frame(const Frame *frame)
{
    if (!frame_is_for_local_device(frame)) {
        printf("Frame ignored: destination 0x%02X\n", frame->destination);
        return;
    }

    if (!frame_content_is_valid(frame)) {
        printf("Frame rejected: invalid command or payload length\n");
        return;
    }

    int16_t value;

    if (!payload_read_i16_le(frame->payload, frame->length, 0u, &value)) {
        printf("Frame rejected: invalid payload\n");
        return;
    }

    printf("Frame accepted\n");
    printf("Source:      0x%02X\n", frame->source);
    printf("Destination: 0x%02X\n", frame->destination);
    printf("Sequence:    %u\n", frame->sequence);
    printf("Command:     0x%02X\n", frame->command);
    printf("Value:       %d\n", value);
}

static void feed_uart(const uint8_t *data, size_t length)
{
    for (size_t i = 0u; i < length; ++i) {
        uart_rx_isr(data[i], 0u);
    }
}

static void consume_uart(Parser *parser)
{
    uint8_t byte;
    Frame frame;

    while (uart_rx_pop(&byte)) {
        if (parser_push(parser, byte, &frame)) {
            process_frame(&frame);
        }
    }
}

int main(void)
{
    uart_rx_init();

    Parser parser;
    parser_init(&parser);

    Frame tx_frame = {
        .destination = LOCAL_ADDRESS,
        .source = 0x01u,
        .sequence = 42u,
        .command = CMD_SET_VALUE,
        .length = 2u,
        .payload = {0u}
    };

    payload_write_i16_le(tx_frame.payload, tx_frame.length, 0u, -1234);

    uint8_t encoded[FRAME_MAX_SIZE];
    size_t encoded_length = protocol_encode(&tx_frame, encoded, sizeof(encoded));

    if (encoded_length == 0u) {
        return 1;
    }

    printf("Valid frame test\n");
    feed_uart(encoded, encoded_length);
    consume_uart(&parser);

    printf("\nCorrupted frame test\n");
    encoded[encoded_length - 1u] ^= 0x01u;
    feed_uart(encoded, encoded_length);
    consume_uart(&parser);

    printf("No frame accepted: CRC validation rejected the corrupted message\n");

    if (uart_rx_overflowed()) {
        printf("RX ring buffer overflow detected\n");
        uart_rx_clear_overflow();
    }

    return 0;
}
