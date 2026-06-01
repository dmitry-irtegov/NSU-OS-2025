#ifndef PROXY_H
#define PROXY_H

#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define LISTEN_BACKLOG 128
#define READ_CHUNK 8192
#define MAX_HEADER_SIZE 65536

typedef enum {
    CACHE_LOADING = 0,
    CACHE_READY,
    CACHE_FAILED
} CacheState;

typedef struct Buffer {
    char *data;
    size_t len;
    size_t cap;
    size_t sent;
} Buffer;

typedef struct HttpRequest {
    char *method;
    char *host;
    char *path;
    char *version;
    char *key;
    int port;
} HttpRequest;

typedef struct CacheObject {
    char *key;
    Buffer response;

    int header_ready;
    size_t header_len;
    int status_code;
    int cacheable;
    int ref_count;

    CacheState state;
    pthread_cond_t ready;

    struct CacheObject *next;
} CacheObject;

/* buffer.c */
void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_clear(Buffer *b);
int buffer_reserve(Buffer *b, size_t need);
int buffer_append(Buffer *b, const void *src, size_t n);
size_t buffer_unsent_size(const Buffer *b);
char *buffer_unsent_ptr(Buffer *b);
void buffer_mark_sent(Buffer *b, size_t n);
void buffer_drop_sent(Buffer *b);

/* net.c */
int create_listener(const char *port);
int start_connect(const char *host, int port);

/* http.c */
ssize_t find_header_end(const char *buf, size_t len);
const char *find_crlf(const char *buf, size_t len);
int parse_http_request(const char *buf, size_t len, HttpRequest *r, int *http_err);
char *make_cache_key(const char *host, int port, const char *path);
int build_origin_request(const HttpRequest *r, Buffer *out);
void request_free(HttpRequest *r);

/* cache.c */
CacheObject *cache_get_or_reserve(const char *key, int *is_owner);
void cache_store_success(CacheObject *obj, Buffer *response);
void cache_store_error(CacheObject *obj, int code, const char *reason, const char *body);
void cache_release(CacheObject *obj);
void cache_parse_header(CacheObject *obj);
void cache_free_all();

#endif
