#include "cache.h"

#include <stdlib.h>
#include <string.h>

int cache_init(cache_t* cache, int capacity) {
    cache->entries = malloc(capacity * sizeof(cache_entry_t));
    if (cache->entries == NULL) {
        return 1;
    }
    cache->capacity = capacity;
    cache->count = 0;
    cache->next = 0;
    pthread_mutex_init(&cache->lock, NULL);
    for (int i = 0; i < capacity; i++) {
        cache->entries[i].key = NULL;
        cache->entries[i].data = NULL;
        cache->entries[i].size = 0;
        cache->entries[i].refs = 0;
    }
    return 0;
}

void cache_destroy(cache_t* cache) {
    for (int i = 0; i < cache->capacity; i++) {
        free(cache->entries[i].key);
        free(cache->entries[i].data);
    }
    free(cache->entries);
    cache->entries = NULL;
    pthread_mutex_destroy(&cache->lock);
}

cache_entry_t* cache_get(cache_t* cache, char* key) {
    pthread_mutex_lock(&cache->lock);

    cache_entry_t* found = NULL;
    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].key, key) == 0) {
            cache->entries[i].refs++;
            found = &cache->entries[i];
            break;
        }
    }

    pthread_mutex_unlock(&cache->lock);
    return found;
}

void cache_release(cache_t* cache, cache_entry_t* entry) {
    if (entry == NULL) {
        return;
    }
    pthread_mutex_lock(&cache->lock);
    entry->refs--;
    pthread_mutex_unlock(&cache->lock);
}

void cache_add(cache_t* cache, char* key, char* data, size_t size) {
    char* key_copy = strdup(key);
    if (key_copy == NULL) {
        return;
    }

    char* data_copy = malloc(size);
    if (data_copy == NULL) {
        free(key_copy);
        return;
    }
    memcpy(data_copy, data, size);

    pthread_mutex_lock(&cache->lock);

    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].key, key) == 0) {
            pthread_mutex_unlock(&cache->lock);
            free(key_copy);
            free(data_copy);
            return;
        }
    }

    int slot = -1;
    if (cache->count < cache->capacity) {
        slot = cache->count;
        cache->count++;
    } else {
        for (int i = 0; i < cache->capacity; i++) {
            int victim = (cache->next + i) % cache->capacity;
            if (cache->entries[victim].refs == 0) {
                free(cache->entries[victim].key);
                free(cache->entries[victim].data);
                slot = victim;
                cache->next = (victim + 1) % cache->capacity;
                break;
            }
        }
    }

    if (slot < 0) {
        pthread_mutex_unlock(&cache->lock);
        free(key_copy);
        free(data_copy);
        return;
    }

    cache->entries[slot].key = key_copy;
    cache->entries[slot].data = data_copy;
    cache->entries[slot].size = size;
    cache->entries[slot].refs = 0;

    pthread_mutex_unlock(&cache->lock);
}
