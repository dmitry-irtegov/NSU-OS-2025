#ifndef CACHE
#define CACHE

#include <stdlib.h>

typedef struct CacheEntry {
    char *data;
    size_t data_size;
    char *request;
    struct CacheEntry *next;
} CacheEntry;

typedef struct {
    CacheEntry *head;
    int entry_count;
    int max_entry_count;
} Cache;

CacheEntry *create_entry(char *data, size_t data_size, char *request);
CacheEntry *add_entry(Cache *cache, CacheEntry *cache_entry);
CacheEntry *get_entry(Cache *cache, char *request);
void free_entry(CacheEntry *cache_entry);

Cache *init_cache(int max_entry_count);
void free_cache(Cache *cache);
void print_cache(Cache *cache);

#endif // CACHE
