#include "upstream.h"
#include "cache.h"
#include "client.h"
#include "utils.h"

Upstream upstreams[MAX_FDS];

int upstream_connect(const char *host, int port, const char *path, int cache_idx) {
    struct sockaddr_in addr;
    if (resolve_host(host, port, &addr) < 0) {
        return -1;
    }
    
    int ufd = socket(AF_INET, SOCK_STREAM, 0);
    if (ufd < 0) return -1;
    
    set_nonblock(ufd);
    
    int uslot = find_free_slot();
    if (uslot < 0) {
        safe_close(ufd);
        return -1;
    }
    
    memset(&upstreams[uslot], 0, sizeof(upstreams[uslot]));
    upstreams[uslot].fd = ufd;
    upstreams[uslot].cache_idx = cache_idx;
    upstreams[uslot].req_len = snprintf(upstreams[uslot].request, 
                                        sizeof(upstreams[uslot].request),
                                        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", 
                                        path, host);
    
    caches[cache_idx].upstream_fd = ufd;
    
    pollfds[uslot].fd = ufd;
    pollfds[uslot].events = POLLOUT | POLLIN;
    fd_register(ufd, uslot, FD_TYPE_UPSTREAM, fd_to_index);
    nfds++;
    connect(ufd, (struct sockaddr*)&addr, sizeof(addr));
    
    return uslot;
}

void upstream_disconnect(int ui) {
    if (ui < 0 || ui >= MAX_FDS) return;
    
    Upstream *up = &upstreams[ui];
    int ci_cache = up->cache_idx;
    
    if (ci_cache >= 0) {
        caches[ci_cache].upstream_fd = -1;
        if (caches[ci_cache].state == CS_FETCHING) {
            caches[ci_cache].state = CS_COMPLETE;
            cache_notify_waiters(ci_cache);
        }
    }
    
    safe_close(up->fd);
    fd_unregister(up->fd, fd_to_index);
    up->fd = -1;
    pollfds[ui].fd = -1;
    nfds--;
}

static void upstream_handle_headers(int ui, CacheEntry *ce, Upstream *up) {
    char *end = strstr(up->buf, "\r\n\r\n");
    if (!end || ce->h_len > 0) return;
    
    size_t hlen = (size_t)(end - up->buf) + 4;
    if (hlen >= HEADER_MAX) {
        ce->state = CS_ERROR;
        for (int i = 0; i < ce->w_count; i++) {
            int wfd = ce->waiters[i];
            int wci = fd_to_idx_lookup(wfd, FD_TYPE_CLIENT, fd_to_index);
            if (wci >= 0 && clients[wci].state == C_WAIT_CACHE) {
                clients[wci].send_len = snprintf(clients[wci].send_buf, BUF_SIZE,
                    "HTTP/1.1 502 Bad Gateway\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<h1>502 Bad Gateway</h1>");
                clients[wci].send_pos = 0;
                clients[wci].state = C_SEND_RESP;
                pollfds[wci].events |= POLLOUT;
            }
        }
        upstream_disconnect(ui);
        return;
    }
    
    memcpy(ce->headers, up->buf, hlen);
    ce->headers[hlen] = '\0';
    ce->h_len = hlen;
    
    if (!is_status_200(ce->headers)) {
        ce->state = CS_COMPLETE;  
        ce->body_len = 0;

        for (int i = 0; i < ce->w_count; i++) {
            int wfd = ce->waiters[i];
            int wci = fd_to_idx_lookup(wfd, FD_TYPE_CLIENT, fd_to_index);
            if (wci >= 0 && clients[wci].state == C_WAIT_CACHE) {
                clients[wci].send_len = snprintf(clients[wci].send_buf, BUF_SIZE,
                    "HTTP/1.1 502 Bad Gateway\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<h1>502 Bad Gateway</h1>");
                clients[wci].send_pos = 0;
                clients[wci].state = C_SEND_RESP;
                pollfds[wci].events |= POLLOUT;
            }
        }
    }

    char *cl = strstr(ce->headers, "Content-Length:");
    if (cl) {
        ce->content_len = atol(cl + 15);
    }
    
    memmove(up->buf, up->buf + hlen, up->len - hlen);
    up->len -= hlen;
    
}

static void upstream_buffer_body(int ui, CacheEntry *ce, Upstream *up) {
    if (ce->h_len == 0 || up->len == 0) return;
    
    size_t need = up->len;
    if (ce->content_len > 0) {
        size_t remain = ce->content_len - ce->body_len;
        if (need > remain) need = remain;
    }

    if (ce->body_len + need > BUF_SIZE) {
        size_t new_sz = ce->body_len + need + BUF_SIZE;
        char *new_body = realloc(ce->body, new_sz);
        if (!new_body) {
            ce->state = CS_ERROR;
            return;
        }
        ce->body = new_body;
    }
    
    
    memcpy(ce->body + ce->body_len, up->buf, need);
    ce->body_len += need;
    
    
    memmove(up->buf, up->buf + need, up->len - need);
    up->len -= need;
    
    
    if (ce->content_len > 0 && ce->body_len >= ce->content_len) {
        ce->state = CS_COMPLETE;
    }
}

void upstream_handle_event(int ui, short revents) {
    if (ui < 0 || ui >= MAX_FDS) return;
    
    Upstream *up = &upstreams[ui];
    int ci_cache = up->cache_idx;
    if (ci_cache < 0 || ci_cache >= MAX_CACHE) {
        upstream_disconnect(ui);
        return;
    }
    
    CacheEntry *ce = &caches[ci_cache];
    
    if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
        ce->state = CS_ERROR;
        upstream_disconnect(ui);
        return;
    }
    
    
    if (revents & POLLOUT && ce->state == CS_FETCHING) {
        int soerr = 0;
        socklen_t soerr_len = sizeof(soerr);
        getsockopt(up->fd, SOL_SOCKET, SO_ERROR, &soerr, &soerr_len);
        
        if (soerr != 0) {
            ce->state = CS_ERROR;
            upstream_disconnect(ui);
            return;
        }
        
        
        ssize_t n = send(up->fd, up->request, up->req_len, 0);
        if (n > 0) {
            pollfds[ui].events = POLLIN;
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            ce->state = CS_ERROR;
            upstream_disconnect(ui);
        }
        return;
    }
    
    if (revents & POLLIN) {
        ssize_t n = recv(up->fd, up->buf + up->len, 
                        BUF_SIZE - up->len - 1, 0);
        
        if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            
            if (ce->h_len > 0 && up->len > 0) {
                upstream_buffer_body(ui, ce, up);
            }
            ce->state = (ce->state == CS_ERROR) ? CS_ERROR : CS_COMPLETE;
            upstream_disconnect(ui);
            cache_notify_waiters(ci_cache);
            return;
        }
        
        if (n > 0) up->len += n;
        
        upstream_handle_headers(ui, ce, up);
        
        
        if (ce->h_len > 0) {
            upstream_buffer_body(ui, ce, up);
            
            
            if (ce->content_len > 0 && ce->body_len >= ce->content_len) {
                ce->state = CS_COMPLETE;
                upstream_disconnect(ui);
                cache_notify_waiters(ci_cache);
            }
        }
    }
}