#define _GNU_SOURCE
#include "cache.h"
#include "client.h"
#include "utils.h"

CacheEntry caches[MAX_CACHE];
int ncache = 0;
size_t current_cache_usage = 0;
size_t max_cache_size = MAX_CACHE_SIZE;

static int cache_is_in_use(int idx) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (clients[i].fd >= 0 && clients[i].cache_idx == idx) {
            return 1;
        }
    }
    return 0;
}

bool cache_ensure_space(size_t needed, FILE *log_file) {
    if (needed > max_cache_size) return false;
    
    while (current_cache_usage + needed > max_cache_size) {
        bool evicted = false;
        for (int i = 0; i < ncache; i++) {
            if (caches[i].state == CS_COMPLETE && !cache_is_in_use(i) && caches[i].body) {
                req_log(log_file, "Evicting cache entry idx=%d (size=%zu) to free space", i, caches[i].body_len);
                current_cache_usage -= caches[i].body_len;
                free(caches[i].body);
                caches[i].body = NULL;
                caches[i].body_len = 0;
                caches[i].state = CS_EMPTY; 
                evicted = true;
                break; 
            }
        }
        if (!evicted) return false;
    }
    return true;
}

int cache_get_or_create(const char *url, FILE *log_file) {
    for (int i = 0; i < ncache; i++) {
        if (strcmp(caches[i].url, url) == 0) {
            return i;
        }
    }
    
    int idx = -1;
    for (int i = 0; i < ncache; i++) {
        if (caches[i].state == CS_EMPTY && caches[i].w_count == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx < 0) {
        if (ncache < MAX_CACHE) {
            idx = ncache++;
        } else {
            for (int i = 0; i < MAX_CACHE; i++) {
                if (caches[i].state == CS_COMPLETE && !cache_is_in_use(i)) {
                    idx = i;
                    req_log(log_file, "Evicting cache entry idx=%d to make room for new URL", idx);
                    current_cache_usage -= caches[i].body_len;
                    free(caches[i].body);
                    caches[i].body = NULL;
                    caches[i].body_len = 0;
                    break;
                }
            }
            if (idx < 0) {
                for (int i = 0; i < MAX_CACHE; i++) {
                    if (caches[i].state == CS_EMPTY) {
                        idx = i;
                        break;
                    }
                }
            }
        }
    }
    
    if (idx < 0) return -1; 
    
    if (caches[idx].body) {
        free(caches[idx].body);
    }
    
    memset(&caches[idx], 0, sizeof(caches[idx]));
    strncpy(caches[idx].url, url, sizeof(caches[idx].url) - 1);
    caches[idx].state = CS_EMPTY;
    
    req_log(log_file, "Cache entry created/reused: idx=%d, url=%s", idx, url);
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

void cache_wake_waiters(int cache_idx) {
    if (cache_idx < 0 || cache_idx >= MAX_CACHE) return;
    CacheEntry *ce = &caches[cache_idx];
    
    for (int i = 0; i < ce->w_count; i++) {
        int wfd = ce->waiters[i];
        int wci = fd_to_idx_lookup(wfd, FD_TYPE_CLIENT, fd_to_index);
        if (wci >= 0) {
            Client *wcl = &clients[wci];
            if (wcl->state == C_SEND_BODY && (wcl->cache_offset < ce->body_len || ce->state == CS_COMPLETE)) {
                pollfds[wci].events |= POLLOUT;
            }
        }
    }
}

void cache_cleanup_all(void) {
    for (int i = 0; i < ncache; i++) {
        if (caches[i].body) {
            free(caches[i].body);
            caches[i].body = NULL;
        }
    }
    ncache = 0;
    current_cache_usage = 0;
}