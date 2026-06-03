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
    pthread_rwlock_t rwlock;
    int in_use;
} CacheEntry;

extern CacheEntry caches[MAX_CACHE];
extern int ncache;
extern size_t total_cached_size;
extern pthread_mutex_t cache_list_mutex;

int cache_get_or_create(const char *url, FILE *log);
size_t cache_evict(size_t needed_size, FILE *log);
void cache_cleanup_all(void);
void cache_cleanup_all_safe(void);

#endif