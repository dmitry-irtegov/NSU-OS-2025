#include "proxy.h"

static CacheObject *cache_head = NULL;
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static CacheObject *cache_find_locked(const char *key) {
    if (key == NULL) {
        return NULL;
    }
    
    for (CacheObject *cur = cache_head; cur != NULL; cur = cur->next) {
        if (cur->key != NULL && strcmp(cur->key, key) == 0) {
            return cur;
        }
    }

    return NULL;
}

static CacheObject *cache_create_locked(const char *key) {
    CacheObject *obj = calloc(1, sizeof(*obj));
    if (obj == NULL) {
        return NULL;
    }

    obj->key = strdup(key);
    if (obj->key == NULL) {
        free(obj);
        return NULL;
    }

    buffer_init(&obj->response);

    obj->header_ready = 0;
    obj->header_len = 0;
    obj->status_code = 0;
    obj->cacheable = 1;
    obj->ref_count = 1;
    obj->state = CACHE_LOADING;

    if (pthread_cond_init(&obj->ready, NULL) != 0) {
        buffer_free(&obj->response);
        free(obj->key);
        free(obj);
        return NULL;
    }

    obj->next = cache_head;
    cache_head = obj;

    return obj;
}

CacheObject *cache_get_or_reserve(const char *key, int *is_owner) {
    if (is_owner != NULL) {
        *is_owner = 0;
    }

    if (key == NULL) {
        return NULL;
    }

    if (pthread_mutex_lock(&cache_mutex) != 0) {
        return NULL;
    }

    CacheObject *obj = cache_find_locked(key);
    if (obj != NULL) {
        obj->ref_count++;

        while (obj->state == CACHE_LOADING) {
            pthread_cond_wait(&obj->ready, &cache_mutex);
        }

        pthread_mutex_unlock(&cache_mutex);
        fprintf(stderr, "CACHE HIT: %s\n", key);
        return obj;
    }

    obj = cache_create_locked(key);
    if (obj == NULL) {
        pthread_mutex_unlock(&cache_mutex);
        return NULL;
    }

    if (is_owner != NULL) {
        *is_owner = 1;
    }

    pthread_mutex_unlock(&cache_mutex);
    fprintf(stderr, "CACHE MISS: %s\n", key);
    return obj;
}

void cache_parse_header(CacheObject *obj) {
    if (obj == NULL || obj->header_ready) {
        return;
    }

    ssize_t header_end = find_header_end(obj->response.data, obj->response.len);
    if (header_end < 0) {
        return;
    }

    obj->header_ready = 1;
    obj->header_len = (size_t)header_end;

    const char *line_start = obj->response.data;
    const char *line_end = find_crlf(obj->response.data, (size_t)header_end);
    
    if (line_end == NULL) {
        obj->cacheable = 0;
        return;
    }

    if (line_end - line_start < 12) {
        obj->cacheable = 0;
        return;
    }

    if (memcmp(line_start, "HTTP/", 5) != 0) {
        obj->cacheable = 0;
        return;
    }

    const char *p = line_start;

    while (p < line_end && *p != ' ') {
        p++;
    }

    while (p < line_end && *p == ' ') {
        p++;
    }

    if (p + 3 > line_end ||
        !isdigit((unsigned char)p[0]) ||
        !isdigit((unsigned char)p[1]) ||
        !isdigit((unsigned char)p[2])) {
        obj->cacheable = 0;
        return;
    }

    int code = (p[0] - '0') * 100 + (p[1] - '0') * 10 + (p[2] - '0');

    obj->status_code = code;

    if (code != 200) {
        obj->cacheable = 0;
    }
}

void cache_store_success(CacheObject *obj, Buffer *response) {
    if (obj == NULL || response == NULL) {
        return;
    }

    pthread_mutex_lock(&cache_mutex);

    buffer_free(&obj->response);

    obj->response = *response;
    buffer_init(response);

    obj->header_ready = 0;
    obj->header_len = 0;
    obj->status_code = 0;
    obj->cacheable = 1;

    cache_parse_header(obj);

    if (!obj->cacheable) {
        obj->state = CACHE_FAILED;
    } else {
        obj->state = CACHE_READY;
    }

    pthread_cond_broadcast(&obj->ready);
    pthread_mutex_unlock(&cache_mutex);
}

void cache_store_error(CacheObject *obj, int code, const char *reason, const char *body) {
    char header[1024];

    if (obj == NULL) {
        return;
    }

    if (reason == NULL) {
        reason = "Error";
    }

    if (body == NULL) {
        body = "";
    }

    pthread_mutex_lock(&cache_mutex);

    buffer_clear(&obj->response);

    int n = snprintf(header, sizeof(header),
                 "HTTP/1.0 %d %s\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n",
                 code,
                 reason,
                 strlen(body));

    if (n > 0 && (size_t)n < sizeof(header)) {
        buffer_append(&obj->response, header, (size_t)n);
        buffer_append(&obj->response, body, strlen(body));
    }

    obj->header_ready = 1;
    obj->header_len = (n > 0) ? (size_t)n : 0;
    obj->status_code = code;
    obj->cacheable = 0;
    obj->state = CACHE_FAILED;

    pthread_cond_broadcast(&obj->ready);
    pthread_mutex_unlock(&cache_mutex);
}

void cache_release(CacheObject *obj) {
    CacheObject **pp;

    if (obj == NULL) {
        return;
    }

    pthread_mutex_lock(&cache_mutex);

    obj->ref_count--;

    if (obj->ref_count == 0 && obj->state != CACHE_READY) {
        pp = &cache_head;

        while (*pp != NULL && *pp != obj) {
            pp = &(*pp)->next;
        }

        if (*pp == obj) {
            *pp = obj->next;
        }

        pthread_cond_destroy(&obj->ready);
        free(obj->key);
        buffer_free(&obj->response);
        free(obj);
    }

    pthread_mutex_unlock(&cache_mutex);
}

void cache_free_all() {
    pthread_mutex_lock(&cache_mutex);

    CacheObject *cur = cache_head;
    CacheObject *next;

    while (cur != NULL) {
        next = cur->next;

        pthread_cond_destroy(&cur->ready);
        free(cur->key);
        buffer_free(&cur->response);
        free(cur);

        cur = next;
    }

    cache_head = NULL;

    pthread_mutex_unlock(&cache_mutex);
}
