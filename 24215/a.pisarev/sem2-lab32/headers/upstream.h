#ifndef PROXY_UPSTREAM_H
#define PROXY_UPSTREAM_H

#include "common.h"
#include "cache.h"

int upstream_fetch(const char *host, int port, const char *path, int cache_idx, int client_fd, FILE *log);

#endif /* PROXY_UPSTREAM_H */
