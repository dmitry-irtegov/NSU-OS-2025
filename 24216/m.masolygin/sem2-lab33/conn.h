#ifndef CONN_H
#define CONN_H

#include <stddef.h>

#include "cache.h"
#include "request.h"

#define REQ_MAX 8192

typedef enum {
    CONN_READ_REQUEST,
    CONN_CONNECT_ORIGIN,
    CONN_SEND_ORIGIN,
    CONN_STREAM,
    CONN_SEND_CLIENT,
    CONN_DONE
} conn_state_t;

typedef struct {
    int fd;
    int origin_fd;
    conn_state_t state;
    cache_t* cache;

    char reqbuf[REQ_MAX];
    size_t reqlen;
    request_t request;
    char key[REQ_MAX];

    char outbuf[REQ_MAX];
    size_t out_len;
    size_t out_sent;

    char* response;
    size_t response_size;
    size_t response_cap;
    size_t sent;
    size_t header_len;
    size_t content_length;
    int origin_eof;
    cache_entry_t* entry;
    int owns_response;
} conn_t;

conn_t* conn_create(int fd, cache_t* cache);
void conn_destroy(conn_t* conn);

int conn_active_fd(conn_t* conn);
int conn_wants_write(conn_t* conn);
void conn_advance(conn_t* conn);

#endif
