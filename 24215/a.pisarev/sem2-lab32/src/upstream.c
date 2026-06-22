#include <sys/time.h>
#include <stdbool.h>
#include "upstream.h"
#include "cache.h"
#include "utils.h"

int is_status_200(const char *headers) {
    return (strstr(headers, "HTTP/1.1 200") != NULL ||
            strstr(headers, "HTTP/1.0 200") != NULL);
}

int upstream_fetch(const char *host, int port, const char *path, int cache_idx, int client_fd, FILE *log) {
    struct sockaddr_in addr;
    if (resolve_host(host, port, &addr) < 0) {
        log_msg(log, "[UPSTREAM] resolve failed host=%s port=%d\n", host, port);
        return -1;
    }
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    
    struct timeval tv = {5, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_msg(log, "[UPSTREAM] connect failed host=%s port=%d errno=%d, %s\n", host, port, errno, strerror(errno));
        safe_close(fd);
        return -1;
    }
    log_msg(log, "[UPSTREAM] connected to %s:%d fd=%d\n", host, port, fd);
    
    char request[1024];
    int req_len = snprintf(request, sizeof(request),
                           "GET %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Connection: close\r\n\r\n",
                           path, host);
                           
    if (send(fd, request, req_len, 0) < 0) {
        log_msg(log, "[UPSTREAM] send request failed errno=%d,%s\n", errno, strerror(errno));
        safe_close(fd);
        return -1;
    }
    
    CacheEntry *ce = (cache_idx >= 0) ? &caches[cache_idx] : NULL;
    char local_headers[HEADER_MAX];
    char *h_dest = ce ? ce->headers : local_headers;
    
    char buf[BUF_SIZE];
    char headers_buf[HEADER_MAX];
    size_t hlen = 0;
    bool headers_parsed = false;
    char leftover[BUF_SIZE];
    size_t leftover_len = 0;
    
    while (!headers_parsed) {
        ssize_t n = recv(fd, buf, BUF_SIZE, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            log_msg(log, "[UPSTREAM] recv headers error errno=%d, %s\n", errno, strerror(errno));
            safe_close(fd);
            return -1;
        }
        if (n == 0) break;
        
        if (hlen + n >= HEADER_MAX) {
            log_msg(log, "[UPSTREAM] headers too large\n");
            safe_close(fd);
            return -1;
        }
        memcpy(headers_buf + hlen, buf, n);
        hlen += n;
        
        char *end = memmem(headers_buf, hlen, "\r\n\r\n", 4);
        if (end) {
            size_t actual_hlen = (end - headers_buf) + 4;
            leftover_len = hlen - actual_hlen;
            if (leftover_len > 0) {
                memcpy(leftover, end + 4, leftover_len);
            }
            hlen = actual_hlen;
            headers_parsed = true;
        }
    }
    
    if (!headers_parsed) {
        log_msg(log, "[UPSTREAM] failed to parse headers\n");
        safe_close(fd);
        return -1;
    }
    
    memcpy(h_dest, headers_buf, hlen);
    h_dest[hlen] = '\0';
    if (ce) ce->h_len = hlen;
    
    if (!is_status_200(h_dest)) {
        log_msg(log, "[UPSTREAM] non-200 status, streaming without caching\n");
        ce = NULL; //Disable caching
    }
    
    size_t content_len = 0;
    bool has_cl = false;
    char *cl = strstr(h_dest, "Content-Length:");
    if (cl) {
        cl += 15;
        while (*cl == ' ') cl++;
        content_len = atol(cl);
        has_cl = true;
    }
    
    bool do_cache = (ce != NULL) && has_cl;
    if (do_cache) {
        if (content_len > MAX_CACHE_SIZE) {
            log_msg(log, "[UPSTREAM] CL=%zu > MAX_CACHE_SIZE, streaming\n", content_len);
            do_cache = false;
        } else {
            pthread_mutex_lock(&cache_list_mutex);
            size_t available = MAX_CACHE_SIZE - total_cached_size;
            pthread_mutex_unlock(&cache_list_mutex);
            
            if (content_len > available) {
                size_t needed = content_len - available;
                size_t freed = cache_evict(needed, log);
                if (freed < needed) {
                    log_msg(log, "[UPSTREAM] could not evict enough space (needed=%zu, freed=%zu), streaming\n", needed, freed);
                    do_cache = false;
                }
            }
        }
    }
    
    if (!do_cache && ce) {
        pthread_rwlock_wrlock(&ce->rwlock);
        ce->state = CS_EMPTY;
        ce->in_use = 0;
        pthread_rwlock_unlock(&ce->rwlock);
        ce = NULL;
    }
    
    // Send headers to client
    if (send(client_fd, h_dest, hlen, 0) < 0) {
        log_msg(log, "[UPSTREAM] send headers to client failed errno=%d, %s\n", errno, strerror(errno));
        safe_close(fd);
        return -1;
    }
    
    if (ce && do_cache) {
        ce->body = malloc(content_len);
        if (!ce->body) {
            log_msg(log, "[UPSTREAM] malloc failed for body size=%zu\n", content_len);
            safe_close(fd);
            return -1;
        }
        ce->body_len = 0;
        ce->content_len = content_len;
        
        if (leftover_len > 0) {
            memcpy(ce->body, leftover, leftover_len);
            ce->body_len = leftover_len;
        }
        
        while (ce->body_len < content_len) {
            ssize_t n = recv(fd, buf, BUF_SIZE, 0);
            if (n < 0) {
                if (errno == EINTR) continue;
                log_msg(log, "[UPSTREAM] recv body error errno=%d, %s\n", errno, strerror(errno));
                break;
            }
            if (n == 0) break;
            
            size_t un = (size_t)n;
            size_t to_copy = (ce->body_len + un > content_len) ? (content_len - ce->body_len) : un;
            memcpy(ce->body + ce->body_len, buf, to_copy);
            ce->body_len += to_copy;
        }
        
        pthread_mutex_lock(&cache_list_mutex);
        total_cached_size += ce->body_len;
        pthread_mutex_unlock(&cache_list_mutex);
        
        if (send(client_fd, ce->body, ce->body_len, 0) < 0) {
            log_msg(log, "[UPSTREAM] send cached body to client failed errno=%d, %s\n", errno, strerror(errno));
        }
    } else {
        // Streaming
        if (leftover_len > 0) {
            if (send(client_fd, leftover, leftover_len, 0) < 0) {
                if (errno != EINTR && errno != EPIPE) {
                    log_msg(log, "[UPSTREAM] send leftover to client failed errno=%d, %s\n", errno, strerror(errno));
                }
                safe_close(fd);
                return -1;
            }
        }
        
        while (1) {
            ssize_t n = recv(fd, buf, BUF_SIZE, 0);
            if (n < 0) {
                if (errno == EINTR) continue;
                log_msg(log, "[UPSTREAM] recv stream error errno=%d,%s\n", errno, strerror(errno));
                break;
            }
            if (n == 0) break;
            
            size_t sent = 0;
            size_t un = (size_t)n;
            while (sent < un) {
                ssize_t s = send(client_fd, buf + sent, un - sent, 0);
                if (s < 0) {
                    if (errno == EINTR) continue;
                    log_msg(log, "[UPSTREAM] send stream to client failed errno=%d, %s\n", errno, strerror(errno));
                    safe_close(fd);
                    return -1;
                }
                if (s == 0) break;
                sent += s;
            }
            if (sent < un) break;
        }
        log_msg(log, "[UPSTREAM] done (streamed)\n");
        safe_close(fd);
        return 1;
    }
    
    log_msg(log, "[UPSTREAM] done (cached)\n");
    safe_close(fd);
    return 0;
}
