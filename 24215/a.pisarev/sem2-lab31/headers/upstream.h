#ifndef PROXY_UPSTREAM_H
#define PROXY_UPSTREAM_H

#include "common.h"
typedef struct {
    int fd;
    char buf[BUF_SIZE];
    size_t len;
    int cache_idx;
    char request[1024];
    size_t req_len;
} Upstream;

extern Upstream upstreams[MAX_FDS];
extern int is_status_200(const char *headers);

int upstream_connect(const char *host, int port, const char *path, int cache_idx);
void upstream_handle_event(int ui, short revents);
void upstream_disconnect(int ui);

#endif /* PROXY_UPSTREAM_H */
