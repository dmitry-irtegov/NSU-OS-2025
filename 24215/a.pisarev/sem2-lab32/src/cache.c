#include "cache.h"
#include "utils.h"

CacheEntry caches[MAX_CACHE];
int ncache = 0;
size_t total_cached_size = 0;
pthread_mutex_t cache_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static int cache_is_in_use(int idx) {
    return caches[idx].in_use;
}


size_t cache_evict(size_t needed_size, FILE* log) {
    size_t freed = 0;
    pthread_mutex_lock(&cache_list_mutex);
    for (int i = 0; i < ncache && freed < needed_size; i++) {
        if (caches[i].state == CS_COMPLETE && caches[i].body) {
            if (pthread_rwlock_trywrlock(&caches[i].rwlock) == 0) {
                size_t entry_size = caches[i].body_len;
                total_cached_size -= entry_size;
                free(caches[i].body);
                caches[i].body = NULL;
                caches[i].body_len = 0;
                caches[i].state = CS_EMPTY;
                caches[i].in_use = 0;
                pthread_rwlock_unlock(&caches[i].rwlock);
                
                freed += entry_size;
                log_msg(log, "[CACHE] Evicted idx=%d, freed=%zu\n", i, entry_size);
            }
        }
    }
    pthread_mutex_unlock(&cache_list_mutex);
    return freed;
}


int cache_get_or_create(const char *url, FILE *log) {
    pthread_mutex_lock(&cache_list_mutex);
    for (int i = 0; i < ncache; i++) {
        if (cache_is_in_use(i) && strcmp(caches[i].url, url) == 0) {
            int idx = i;
            log_msg(log, "[CACHE] hit url=%s idx=%d\n", url, idx);
            pthread_mutex_unlock(&cache_list_mutex);
            return idx;
        }
    }
    
    int idx = -1;
    for (int i = 0; i < ncache; i++) {
        if ((caches[i].state == CS_EMPTY || caches[i].state == CS_ERROR|| caches[i].state == CS_STREAMED) && !cache_is_in_use(i)) {
            idx = i;
            log_msg(log, "[CACHE] Reusing empty/error/streaming entry idx=%d\n", idx);
            break;
        }
    }
    
    if (idx < 0 && ncache < MAX_CACHE) {
        idx = ncache++;
    }
    
    if (idx < 0) {
        log_msg(log, "[CACHE] Cache full, no evictable entries\n");
        pthread_mutex_unlock(&cache_list_mutex);
        return -1;
    }
    
    memset(&caches[idx], 0, sizeof(caches[idx]));
    strncpy(caches[idx].url, url, sizeof(caches[idx].url) - 1);
    caches[idx].state = CS_EMPTY;
    caches[idx].in_use = 1;
    pthread_rwlock_init(&caches[idx].rwlock, NULL);
    
    log_msg(log, "[CACHE] miss, created entry idx=%d url=%s\n", idx, url);
    pthread_mutex_unlock(&cache_list_mutex);
    return idx;
}

void cache_cleanup_all(void) {
    log_msg(stderr, "[CACHE] cleaning up %d entries\n", ncache);
    for (int i = 0; i < ncache; i++) {
        if (caches[i].in_use || caches[i].body) {
            free(caches[i].body);
            pthread_rwlock_destroy(&caches[i].rwlock);
        }
    }
    ncache = 0;
    total_cached_size = 0;
}

