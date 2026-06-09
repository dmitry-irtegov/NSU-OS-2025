#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <stddef.h>

typedef struct {
    char* key;
    char* data;
    size_t size;
    int refs;
} cache_entry_t;

typedef struct {
    cache_entry_t* entries;
    int capacity;
    int count;
    int next;
    pthread_mutex_t lock;
} cache_t;

int cache_init(cache_t* cache, int capacity);
void cache_destroy(cache_t* cache);
cache_entry_t* cache_get(cache_t* cache, char* key);
void cache_release(cache_t* cache, cache_entry_t* entry);
void cache_add(cache_t* cache, char* key, char* data, size_t size);

#endif
