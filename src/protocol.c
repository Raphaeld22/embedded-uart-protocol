#include "protocol.h"

#include <string.h>

static void parser_restart(Parser *parser, uint8_t byte)
{
    parser_init(parser);

    if (byte == FRAME_SOF1) {
        parser->state = PARSER_WAIT_SOF2;
    }
}

uint16_t crc16_update(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)byte << 8;

    for (uint8_t i = 0u; i < 8u; ++i) {
        if ((crc & 0x8000u) != 0u) {
            crc = (uint16_t)((crc << 1) ^ 0x1021u);
        } else {
            crc <<= 1;
        }
    }

    return crc;
}

void parser_init(Parser *parser)
{
    if (parser == NULL) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    parser->state = PARSER_WAIT_SOF1;
    parser->calculated_crc = 0xFFFFu;
}

bool parser_push(Parser *parser, uint8_t byte, Frame *completed_frame)
{
    if (parser == NULL) {
        return false;
    }

    switch (parser->state) {
        case PARSER_WAIT_SOF1:
            if (byte == FRAME_SOF1) {
                parser->state = PARSER_WAIT_SOF2;
            }
            break;

        case PARSER_WAIT_SOF2:
            if (byte == FRAME_SOF2) {
                parser->calculated_crc = 0xFFFFu;
                parser->state = PARSER_DESTINATION;
            } else {
                parser_restart(parser, byte);
            }
            break;

        case PARSER_DESTINATION:
            parser->frame.destination = byte;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte);
            parser->state = PARSER_SOURCE;
            break;

        case PARSER_SOURCE:
            parser->frame.source = byte;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte);
            parser->state = PARSER_SEQUENCE;
            break;

        case PARSER_SEQUENCE:
            parser->frame.sequence = byte;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte);
            parser->state = PARSER_COMMAND;
            break;

        case PARSER_COMMAND:
            parser->frame.command = byte;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte);
            parser->state = PARSER_LENGTH;
            break;

        case PARSER_LENGTH:
            parser->frame.length = byte;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte);
            parser->payload_index = 0u;

            if (byte > FRAME_MAX_PAYLOAD) {
                parser_restart(parser, byte);
            } else if (byte == 0u) {
                parser->state = PARSER_CRC_LOW;
            } else {
                parser->state = PARSER_PAYLOAD;
            }
            break;

        case PARSER_PAYLOAD:
            parser->frame.payload[parser->payload_index++] = byte;
            parser->calculated_crc = crc16_update(parser->calculated_crc, byte);

            if (parser->payload_index >= parser->frame.length) {
                parser->state = PARSER_CRC_LOW;
            }
            break;

        case PARSER_CRC_LOW:
            parser->received_crc = byte;
            parser->state = PARSER_CRC_HIGH;
            break;

        case PARSER_CRC_HIGH:
            parser->received_crc |= (uint16_t)byte << 8;

            if (parser->received_crc == parser->calculated_crc) {
                if (completed_frame != NULL) {
                    *completed_frame = parser->frame;
                }

                parser_init(parser);
                return true;
            }

            parser_restart(parser, byte);
            break;

        default:
            parser_init(parser);
            break;
    }

    return false;
}

size_t protocol_encode(const Frame *frame, uint8_t *output, size_t capacity)
{
    if (frame == NULL || output == NULL || frame->length > FRAME_MAX_PAYLOAD) {
        return 0u;
    }

    size_t required = 2u + 5u + (size_t)frame->length + 2u;

    if (capacity < required) {
        return 0u;
    }

    size_t index = 0u;
    uint16_t crc = 0xFFFFu;

    output[index++] = FRAME_SOF1;
    output[index++] = FRAME_SOF2;

    output[index++] = frame->destination;
    crc = crc16_update(crc, frame->destination);

    output[index++] = frame->source;
    crc = crc16_update(crc, frame->source);

    output[index++] = frame->sequence;
    crc = crc16_update(crc, frame->sequence);

    output[index++] = frame->command;
    crc = crc16_update(crc, frame->command);

    output[index++] = frame->length;
    crc = crc16_update(crc, frame->length);

    for (uint8_t i = 0u; i < frame->length; ++i) {
        output[index++] = frame->payload[i];
        crc = crc16_update(crc, frame->payload[i]);
    }

    output[index++] = (uint8_t)(crc & 0x00FFu);
    output[index++] = (uint8_t)(crc >> 8);

    return index;
}

bool payload_read_i16_le(const uint8_t *payload, size_t length, size_t offset, int16_t *value)
{
    if (payload == NULL || value == NULL || offset + 1u >= length) {
        return false;
    }

    uint16_t raw = (uint16_t)payload[offset] |
                   ((uint16_t)payload[offset + 1u] << 8);

    int32_t signed_value = (int32_t)raw;

    if (signed_value >= 0x8000) {
        signed_value -= 0x10000;
    }

    *value = (int16_t)signed_value;
    return true;
}

bool payload_write_i16_le(uint8_t *payload, size_t length, size_t offset, int16_t value)
{
    if (payload == NULL || offset + 1u >= length) {
        return false;
    }

    uint16_t raw = (uint16_t)value;
    payload[offset] = (uint8_t)(raw & 0x00FFu);
    payload[offset + 1u] = (uint8_t)(raw >> 8);
    return true;
}
