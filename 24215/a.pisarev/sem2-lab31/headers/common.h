#ifndef PROXY_COMMON_H
#define PROXY_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <stdatomic.h>

#define MAX_FDS         128
#define MAX_CACHE       64
#define BUF_SIZE        8192
#define HEADER_MAX      4096
#define URL_SIZE        1024
#define PROXY_PORT      8080

#define FD_TYPE_CLIENT    1
#define FD_TYPE_UPSTREAM  2
#define FD_INDEX_ENCODE(idx, type)  (((type) << 16) | ((idx) & 0xFFFF))
#define FD_INDEX_DECODE_IDX(val)    ((val) & 0xFFFF)
#define FD_INDEX_DECODE_TYPE(val)   ((val) >> 16)

typedef enum {
    C_PARSE_REQ,
    C_WAIT_CACHE,
    C_SEND_RESP
} ClientState;

typedef enum {
    CS_EMPTY,
    CS_FETCHING,
    CS_COMPLETE,
    CS_ERROR
} CacheState;


extern struct pollfd pollfds[MAX_FDS];
extern int nfds;
extern volatile sig_atomic_t running;


static inline int fd_to_idx_lookup(int fd, int type, int *fd_to_index);
static inline void fd_register(int fd, int idx, int type, int *fd_to_index);
static inline void fd_unregister(int fd, int *fd_to_index);

#include "utils.h"

#endif /* PROXY_COMMON_H */
