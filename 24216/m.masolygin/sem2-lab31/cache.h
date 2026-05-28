#include <stdlib.h>

typedef struct cache_entry {
    char* body;
    size_t body_len;
    char* key;
    struct cache_entry* next;
} cache_entry_t;

typedef struct cache {
    cache_entry_t* sentinel;
    cache_entry_t* tail;
    int count;
    int capacity;
} cache_t;

cache_entry_t* cache_entry_new(char* body, size_t body_len, char* key);
void cache_entry_free(cache_entry_t* entry);
cache_entry_t* cache_put(cache_t* cache, cache_entry_t* entry);
cache_entry_t* cache_get(cache_t* cache, char* key);

cache_t* cache_new(int capacity);
void cache_free(cache_t* cache);
