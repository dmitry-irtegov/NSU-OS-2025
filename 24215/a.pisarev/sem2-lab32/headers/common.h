#ifndef PROXY_COMMON_H
#define PROXY_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <stdarg.h>

#define MAX_THREADS     128
#define MAX_CACHE       1024
#define BUF_SIZE        16384
#define HEADER_MAX      8192
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
    CS_EMPTY,
    CS_FETCHING,
    CS_COMPLETE,
    CS_ERROR,
    CS_STREAMED
} CacheState;


typedef struct {
    int fd;
    char req_buf[BUF_SIZE];
    size_t req_len;
    char send_buf[BUF_SIZE];
    size_t send_len, send_pos;
    char method[8];
    char url[URL_SIZE];
    char host[256];
    int port;
    char path[URL_SIZE];
    FILE *log;
    unsigned long req_id;
} Client;

extern volatile sig_atomic_t running;
extern int active_threads;
extern pthread_mutex_t threads_mutex;
extern pthread_cond_t threads_cond;

void log_msg(FILE *log, const char *fmt, ...);

#endif /* PROXY_COMMON_H */
