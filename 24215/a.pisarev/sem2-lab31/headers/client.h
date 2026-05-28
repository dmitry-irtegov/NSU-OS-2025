#ifndef PROXY_CLIENT_H
#define PROXY_CLIENT_H

#include "common.h"

typedef struct {
    int fd;
    char req_buf[BUF_SIZE];
    size_t req_len;
    char send_buf[BUF_SIZE];
    size_t send_len, send_pos;
    ClientState state;
    char method[8];
    char url[URL_SIZE];
    int cache_idx;
    size_t cache_offset;
} Client;

extern Client clients[MAX_FDS];

int client_accept(int listen_fd);
void client_handle_event(int ci, short revents);
void client_disconnect(int ci);
void client_send_response_start(int ci);

#endif /* PROXY_CLIENT_H */
