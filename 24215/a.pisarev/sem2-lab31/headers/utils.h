#ifndef PROXY_UTILS_H
#define PROXY_UTILS_H

#include "common.h"


extern int fd_to_index[MAX_FDS * 2];


static inline int fd_to_idx_lookup(int fd, int type, int *fd_to_index) {
    if (fd < 0 || fd >= MAX_FDS * 2) return -1;
    int val = fd_to_index[fd];
    return ((val >> 16) == type) ? (val & 0xFFFF) : -1;
}

static inline void fd_register(int fd, int idx, int type, int *fd_to_index) {
    if (fd >= 0 && fd < MAX_FDS * 2) {
        fd_to_index[fd] = FD_INDEX_ENCODE(idx, type);
    }
}

static inline void fd_unregister(int fd, int *fd_to_index) {
    if (fd >= 0 && fd < MAX_FDS * 2) {
        fd_to_index[fd] = 0;
    }
}


void set_nonblock(int fd);
void safe_close(int fd);
int find_free_slot(void);
void parse_path(const char *url, char *path, size_t path_sz);
int resolve_host(const char *host, int port, struct sockaddr_in *addr);

#endif /* PROXY_UTILS_H */
