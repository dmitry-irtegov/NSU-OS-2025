#include "cache.h"
#include "client.h"
#include "utils.h"

CacheEntry caches[MAX_CACHE];
int ncache = 0;

int cache_get_or_create(const char *url) {
    for (int i = 0; i < ncache; i++) {
        if (strcmp(caches[i].url, url) == 0) {
            return i;
        }
    }
    
    if (ncache >= MAX_CACHE) return -1;
    
    int idx = ncache++;
    memset(&caches[idx], 0, sizeof(caches[idx]));
    strncpy(caches[idx].url, url, sizeof(caches[idx].url) - 1);
    caches[idx].state = CS_EMPTY;
    caches[idx].upstream_fd = -1;
    caches[idx].body = malloc(BUF_SIZE);
    if (!caches[idx].body) { 
        ncache--; 
        return -1; 
    }
    return idx;
}

void cache_add_waiter(int cache_idx, int client_fd) {
    if (cache_idx < 0 || cache_idx >= MAX_CACHE) return;
    CacheEntry *ce = &caches[cache_idx];
    if (ce->w_count < MAX_FDS) {
        ce->waiters[ce->w_count++] = client_fd;
    }
}

void cache_remove_waiter(int cache_idx, int client_fd) {
    if (cache_idx < 0 || cache_idx >= MAX_CACHE) return;
    CacheEntry *ce = &caches[cache_idx];
    for (int i = 0; i < ce->w_count; i++) {
        if (ce->waiters[i] == client_fd) {
            memmove(&ce->waiters[i], &ce->waiters[i+1], 
                   (ce->w_count - i - 1) * sizeof(int));
            ce->w_count--;
            return;
        }
    }
}

void cache_notify_waiters(int cache_idx) {
    if (cache_idx < 0 || cache_idx >= MAX_CACHE) return;
    CacheEntry *ce = &caches[cache_idx];
    
    for (int i = 0; i < ce->w_count; i++) {
        int wfd = ce->waiters[i];
        int wci = fd_to_idx_lookup(wfd, FD_TYPE_CLIENT, fd_to_index);
        if (wci >= 0 && clients[wci].state == C_WAIT_CACHE) {
            clients[wci].state = C_SEND_RESP;
            pollfds[wci].events |= POLLOUT;
        }
    }
}

void cache_cleanup_all(void) {
    for (int i = 0; i < ncache; i++) {
        free(caches[i].body);
        caches[i].body = NULL;
    }
    ncache = 0;
}