#include "cache.h"
#include <stdio.h>
#include <string.h>

CacheEntry *create_entry(char *data, size_t data_size, char *request) {
    CacheEntry *result = malloc(sizeof(CacheEntry));
    result->data = data;
    result->data_size = data_size;
    result->request = request;
    result->next = NULL;

    return result;
}

CacheEntry *add_entry(Cache *cache, CacheEntry *cache_entry) {
    CacheEntry *p, *removed_entry = NULL;

    for (p = cache->head; p->next; p = p->next) {
        if (!strcmp(p->next->request, cache_entry->request)) {
            cache_entry->next = p->next->next;
            removed_entry = p->next;
            p->next = cache_entry;
            return removed_entry;
        }
    }

    p->next = cache_entry;
    if (cache->entry_count == cache->max_entry_count) {
        removed_entry = cache->head->next;
        cache->head->next = cache->head->next->next;
    } else {
        cache->entry_count++;
    }

    return removed_entry;
}

CacheEntry *get_entry(Cache *cache, char *request) {
    for (CacheEntry *p = cache->head->next; p; p = p->next) {
        if (!strcmp(p->request, request)) {
            return p;
        }
    }
    return NULL;
}

void free_entry(CacheEntry *cache_entry) {
    free(cache_entry->data);
    free(cache_entry->request);
    free(cache_entry);
}

Cache *init_cache(int max_entry_count) {
    Cache *result = malloc(sizeof(Cache));
    result->entry_count = 0;
    result->max_entry_count = max_entry_count;
    
    result->head = malloc(sizeof(CacheEntry));
    result->head->next = NULL;
    
    return result;
}

void free_cache(Cache *cache) {
    for (CacheEntry *p = cache->head->next; p;) {
        CacheEntry *next = p->next;
        free_entry(p);
        p = next;
    }

    free(cache->head);
}

void print_cache(Cache *cache) {
    for (CacheEntry *p = cache->head->next; p; p = p->next) {
        printf("[%s]: %s\n\n", p->request, p->data);
    }
}
