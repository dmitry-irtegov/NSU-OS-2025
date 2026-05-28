#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"

int cache_init(cache_t* cache) {
    cache->head = NULL;
    cache->tail = NULL;
    cache->count = 0;

    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_ERRORCHECK);
    int res = pthread_mutex_init(&cache->mtx, &m_attr);
    pthread_mutexattr_destroy(&m_attr);
    if (res != 0) {
        fprintf(stderr, "Error cache mutex init: %s\n", strerror(res));
        return -1;
    }
    return 0;
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

    int res = pthread_mutex_destroy(&cache->mtx);
    if (res != 0) {
        fprintf(stderr, "Error cache mutex destroy: %s\n", strerror(res));
    }
}

int cache_get_copy(cache_t* cache, const char* key, char** out_data, size_t* out_len) {
    int res;
    if ((res = pthread_mutex_lock(&cache->mtx)) != 0) {
        fprintf(stderr, "Error cache lock: %s\n", strerror(res));
        return -1;
    }

    int found = 0;
    cache_entry_t* e = cache->head;
    while (e != NULL) {
        if (strcmp(e->key, key) == 0) {
            char* copy = malloc(e->data_len);
            if (copy == NULL) {
                perror("Error malloc cache copy");
            } else {
                memcpy(copy, e->data, e->data_len);
                *out_data = copy;
                *out_len = e->data_len;
                found = 1;
            }
            break;
        }
        e = e->next;
    }

    if ((res = pthread_mutex_unlock(&cache->mtx)) != 0) {
        fprintf(stderr, "Error cache unlock: %s\n", strerror(res));
    }

    return found;
}

int cache_put(cache_t* cache, const char* key, const char* data, size_t data_len) {
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

    int res;
    if ((res = pthread_mutex_lock(&cache->mtx)) != 0) {
        fprintf(stderr, "Error cache lock: %s\n", strerror(res));
        free(e->data);
        free(e->key);
        free(e);
        return -1;
    }

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

    if (cache->tail == NULL) {
        cache->head = e;
        cache->tail = e;
    } else {
        cache->tail->next = e;
        cache->tail = e;
    }
    cache->count++;

    if ((res = pthread_mutex_unlock(&cache->mtx)) != 0) {
        fprintf(stderr, "Error cache unlock: %s\n", strerror(res));
    }

    return 0;
}
