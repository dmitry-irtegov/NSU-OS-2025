#pragma once

#include <stddef.h>
#include <pthread.h>

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
    pthread_mutex_t mtx;
} cache_t;

int cache_init(cache_t* cache);
void cache_destroy(cache_t* cache);

int cache_get_copy(cache_t* cache, const char* key, char** out_data, size_t* out_len);
int cache_put(cache_t* cache, const char* key, const char* data, size_t data_len);
