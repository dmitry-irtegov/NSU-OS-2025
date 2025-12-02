#include "protocol.h"

void parser_init(struct Parser *pr) {
    pr->state = STATE_WAIT_START;
    pr->len = 0;
    pr->client_id = 0;
}

int protocol_encode(int client_id, const char *data, int len, char *dst) {
    int pos = 0;
    
    dst[pos++] = FRAME_BYTE;

    unsigned char id = (unsigned char)client_id;
    if (id == FRAME_BYTE || id == ESCAPE_BYTE) {
        dst[pos++] = ESCAPE_BYTE;
        dst[pos++] = id;
    } else {
        dst[pos++] = id;
    }

    for (int i = 0; i < len; i++) {
        unsigned char byte = (unsigned char)data[i];
        if (byte == FRAME_BYTE || byte == ESCAPE_BYTE) {
            dst[pos++] = ESCAPE_BYTE;
            dst[pos++] = byte;
        } else {
            dst[pos++] = byte;
        }
    }

    dst[pos++] = FRAME_BYTE;

    return pos;
}

int protocol_decode(struct Parser *pr, char byte_in) {
    unsigned char byte = (unsigned char)byte_in;
    switch (pr->state) {
        case STATE_WAIT_START:
            if (byte == FRAME_BYTE) {
                pr->state = STATE_WAIT_ID;
            }
            break;

        case STATE_WAIT_ID:
            if (byte == ESCAPE_BYTE) {
                pr->state = STATE_ESCAPE_ID;
            } else if (byte == FRAME_BYTE) {
                return 0;
            } else {
                pr->client_id = byte;
                pr->len = 0;
                pr->state = STATE_READ_DATA;
            }
            break;

        case STATE_READ_DATA:
            if (byte == FRAME_BYTE) {
                pr->state = STATE_WAIT_START;
                return 1;
            }

            if (byte == ESCAPE_BYTE) {
                pr->state = STATE_ESCAPE;
            } else {
                if (pr->len < MAX_FRAME_SIZE) {
                    pr->buffer[pr->len++] = byte;
                } else {
                    pr->state = STATE_WAIT_START;
                }
            }
            break;

        case STATE_ESCAPE:
            if (pr->len < MAX_FRAME_SIZE) {
                pr->buffer[pr->len++] = byte;
            }
            pr->state = STATE_READ_DATA;
            break;

        case STATE_ESCAPE_ID:
            pr->client_id = byte;
            pr->len = 0;
            pr->state = STATE_READ_DATA;
            break;            
    }

    return 0;
}