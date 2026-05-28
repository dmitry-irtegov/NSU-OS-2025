#ifndef PROXY_CACHE_H
#define PROXY_CACHE_H

#include "common.h"

typedef struct {
    char url[URL_SIZE];
    CacheState state;
    char headers[HEADER_MAX];
    size_t h_len;
    char *body;
    size_t body_len;
    size_t content_len;
    int upstream_fd;
    int waiters[MAX_FDS];
    int w_count;
} CacheEntry;

extern CacheEntry caches[MAX_CACHE];
extern int ncache;

int cache_get_or_create(const char *url);
void cache_add_waiter(int cache_idx, int client_fd);
void cache_remove_waiter(int cache_idx, int client_fd);
void cache_notify_waiters(int cache_idx);
void cache_cleanup_all(void);

#endif /* PROXY_CACHE_H */
