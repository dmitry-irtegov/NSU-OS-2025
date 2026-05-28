#include "cache.h"

#include <string.h>

cache_entry_t* cache_entry_new(char* body, size_t body_len, char* key) {
    cache_entry_t* entry = malloc(sizeof(cache_entry_t));
    if (!entry) return NULL;
    entry->body = body;
    entry->body_len = body_len;
    entry->key = key;
    entry->next = NULL;
    return entry;
}

void cache_entry_free(cache_entry_t* entry) {
    free(entry->body);
    free(entry->key);
    free(entry);
}

cache_entry_t* cache_put(cache_t* cache, cache_entry_t* entry) {
    cache_entry_t* cur;
    cache_entry_t* removed = NULL;

    for (cur = cache->sentinel; cur->next; cur = cur->next) {
        if (!strcmp(cur->next->key, entry->key)) {
            entry->next = cur->next->next;
            removed = cur->next;
            cur->next = entry;
            if (entry->next == NULL) {
                cache->tail = entry;
            }
            return removed;
        }
    }

    if (cache->count == cache->capacity) {
        removed = cache->sentinel->next;
        cache->sentinel->next = cache->sentinel->next->next;

        if (cache->sentinel->next == NULL) {
            cache->tail = cache->sentinel;
        }

        cur = cache->tail;
    } else {
        cache->count++;
        cur = cache->tail;
    }

    cur->next = entry;
    cache->tail = entry;

    return removed;
}

cache_entry_t* cache_get(cache_t* cache, char* key) {
    for (cache_entry_t* cur = cache->sentinel->next; cur; cur = cur->next) {
        if (!strcmp(cur->key, key)) {
            return cur;
        }
    }
    return NULL;
}

cache_t* cache_new(int capacity) {
    cache_t* cache = malloc(sizeof(cache_t));
    if (!cache) return NULL;
    cache->count = 0;
    cache->capacity = capacity;
    cache->sentinel = malloc(sizeof(cache_entry_t));
    if (!cache->sentinel) {
        free(cache);
        return NULL;
    }
    cache->sentinel->next = NULL;
    cache->tail = cache->sentinel;
    return cache;
}

void cache_free(cache_t* cache) {
    for (cache_entry_t* cur = cache->sentinel->next; cur;) {
        cache_entry_t* next = cur->next;
        cache_entry_free(cur);
        cur = next;
    }
    free(cache->sentinel);
    free(cache);
}
