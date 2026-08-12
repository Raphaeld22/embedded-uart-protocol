#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FRAME_SOF1 0xA5u
#define FRAME_SOF2 0x5Au
#define FRAME_MAX_PAYLOAD 64u
#define FRAME_MAX_SIZE (2u + 5u + FRAME_MAX_PAYLOAD + 2u)
#define FRAME_BROADCAST_ADDRESS 0xFFu

typedef struct {
    uint8_t destination;
    uint8_t source;
    uint8_t sequence;
    uint8_t command;
    uint8_t length;
    uint8_t payload[FRAME_MAX_PAYLOAD];
} Frame;

typedef enum {
    PARSER_WAIT_SOF1 = 0,
    PARSER_WAIT_SOF2,
    PARSER_DESTINATION,
    PARSER_SOURCE,
    PARSER_SEQUENCE,
    PARSER_COMMAND,
    PARSER_LENGTH,
    PARSER_PAYLOAD,
    PARSER_CRC_LOW,
    PARSER_CRC_HIGH
} ParserState;

typedef struct {
    ParserState state;
    Frame frame;
    uint8_t payload_index;
    uint16_t calculated_crc;
    uint16_t received_crc;
} Parser;

void parser_init(Parser *parser);
bool parser_push(Parser *parser, uint8_t byte, Frame *completed_frame);
uint16_t crc16_update(uint16_t crc, uint8_t byte);
size_t protocol_encode(const Frame *frame, uint8_t *output, size_t capacity);
bool payload_read_i16_le(const uint8_t *payload, size_t length, size_t offset, int16_t *value);
bool payload_write_i16_le(uint8_t *payload, size_t length, size_t offset, int16_t value);

#endif
