#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"

void cache_init(cache_t* cache) {
    cache->head = NULL;
    cache->tail = NULL;
    cache->count = 0;
}

void cache_destroy(cache_t* cache) {
    cache_entry_t* e = cache->head;
    while (e != NULL) {
        cache_entry_t* next = e->next;
        free(e->key);
        free(e->data);
        free(e);
        e = next;
    }
    cache->head = NULL;
    cache->tail = NULL;
    cache->count = 0;
}

const cache_entry_t* cache_get(cache_t* cache, const char* key) {
    cache_entry_t* e = cache->head;
    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

int cache_put(cache_t* cache, const char* key, const char* data, size_t data_len) {
    if (cache->count >= MAX_CACHE_ENTRIES) {
        cache_entry_t* evict = cache->head;
        cache->head = evict->next;
        if (cache->head == NULL) {
            cache->tail = NULL;
        }
        free(evict->key);
        free(evict->data);
        free(evict);
        cache->count--;
    }

    cache_entry_t* e = malloc(sizeof(cache_entry_t));
    if (e == NULL) {
        perror("Error malloc cache entry");
        return -1;
    }

    e->key = strdup(key);
    if (e->key == NULL) {
        perror("Error strdup cache key");
        free(e);
        return -1;
    }

    e->data = malloc(data_len);
    if (e->data == NULL) {
        perror("Error malloc cache data");
        free(e->key);
        free(e);
        return -1;
    }

    memcpy(e->data, data, data_len);
    e->data_len = data_len;
    e->next = NULL;

    if (cache->tail == NULL) {
        cache->head = e;
        cache->tail = e;
    } else {
        cache->tail->next = e;
        cache->tail = e;
    }
    cache->count++;

    return 0;
}
