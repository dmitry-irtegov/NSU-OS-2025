#ifndef PROXY_CACHE_H
#define PROXY_CACHE_H
#include "common.h"

int cache_get_or_create(const char *url, FILE *log_file);
bool cache_ensure_space(size_t needed_size, FILE *log_file);
void cache_add_waiter(int cache_idx, int client_fd);
void cache_remove_waiter(int cache_idx, int client_fd);
void cache_wake_waiters(int cache_idx);
void cache_cleanup_all(void);

#endif /* PROXY_CACHE_H */