#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define FRAME_BYTE 0x7E
#define ESCAPE_BYTE 0x7D

#define MAX_FRAME_SIZE 4096

#define STATE_WAIT_START 0
#define STATE_WAIT_ID    1
#define STATE_READ_DATA  2
#define STATE_ESCAPE     3
#define STATE_ESCAPE_ID  4

struct Parser {
    int state;
    char buffer[MAX_FRAME_SIZE];
    int len;
    int client_id;
};

void parser_init(struct Parser *pr);

int protocol_encode(int client_id, const char *data, int len, char *dst);

int protocol_decode(struct Parser *pr, char byte);

#endif // PROTOCOL_H