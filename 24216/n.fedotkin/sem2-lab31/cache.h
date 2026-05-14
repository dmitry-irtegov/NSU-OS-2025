#pragma once

#include <stddef.h>

#define MAX_CACHE_ENTRIES 100

typedef struct cache_entry {
    char* key;
    char* data;
    size_t data_len;
    struct cache_entry* next;
} cache_entry_t;

typedef struct {
    cache_entry_t* head;
    cache_entry_t* tail;
    int count;
} cache_t;

void cache_init(cache_t* cache);
void cache_destroy(cache_t* cache);

const cache_entry_t* cache_get(cache_t* cache, const char* key);
int cache_put(cache_t* cache, const char* key, const char* data, size_t data_len);
