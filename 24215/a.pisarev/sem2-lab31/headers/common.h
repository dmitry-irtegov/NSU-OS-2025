#ifndef PROXY_COMMON_H
#define PROXY_COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_FDS         1024
#define MAX_CACHE       2048
#define BUF_SIZE        8192
#define HEADER_MAX      4096
#define URL_SIZE        1024
#define PROXY_PORT      8080

#define MAX_CACHE_SIZE  (1024ULL * 1024 * 1024 * 2) 
#define LOG_DIR         "logs"

#define FD_TYPE_CLIENT    1
#define FD_TYPE_UPSTREAM  2
#define FD_INDEX_ENCODE(idx, type)  (((type) << 16) | ((idx) & 0xFFFF))
#define FD_INDEX_DECODE_IDX(val)    ((val) & 0xFFFF)
#define FD_INDEX_DECODE_TYPE(val)   ((val) >> 16)

typedef enum {
    C_PARSE_REQ,
    C_WAIT_UPSTREAM,
    C_SEND_HEADERS,
    C_SEND_BODY
} ClientState;

typedef enum {
    CS_EMPTY,
    CS_FETCHING,
    CS_COMPLETE,
    CS_ERROR
} CacheState;

typedef struct {
    int fd;
    FILE *log_file;
    char req_buf[BUF_SIZE];
    size_t req_len;
    char *send_buf;
    size_t send_len, send_pos, send_cap;
    char *stream_buf;
    size_t stream_len, stream_pos, stream_cap;
    ClientState state;
    char method[8];
    char url[URL_SIZE];
    int upstream_idx;
    int cache_idx;
    size_t cache_offset;
    bool stream_mode;
} Client;

typedef struct {
    char url[URL_SIZE];
    CacheState state;
    char headers[HEADER_MAX];
    size_t h_len;
    char *body;
    size_t body_len;
    size_t content_len;
    int waiters[MAX_FDS];
    int w_count;
} CacheEntry;

extern struct pollfd pollfds[MAX_FDS];
extern int nfds;
extern volatile sig_atomic_t running;

extern Client clients[MAX_FDS];
extern CacheEntry caches[MAX_CACHE];
extern int ncache;
extern size_t current_cache_usage;
extern size_t max_cache_size;

#include "utils.h"

#endif /* PROXY_COMMON_H */