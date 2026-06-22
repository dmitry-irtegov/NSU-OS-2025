#define _GNU_SOURCE
#include "upstream.h"
#include "cache.h"
#include "client.h"
#include "utils.h"

Upstream upstreams[MAX_FDS];

int is_status_200(const char *headers) {
    return (strstr(headers, "HTTP/1.") && 
            (strstr(headers, " 200 ") || strstr(headers, " 200\r\n")));
}

int upstream_connect(const char *host, int port, const char *path, int client_idx) {
    struct sockaddr_in addr;
    if (resolve_host(host, port, &addr) < 0) {
        req_log(clients[client_idx].log_file, "DNS resolve failed for %s", host);
        return -1;
    }
    
    int ufd = socket(AF_INET, SOCK_STREAM, 0);
    if (ufd < 0) return -1;
    set_nonblock(ufd);
    int uslot = find_free_slot();
    if (uslot < 0) {
        safe_close(ufd);
        req_log(clients[client_idx].log_file, "No free slots for upstream");
        return -1;
    }
    
    memset(&upstreams[uslot], 0, sizeof(upstreams[uslot]));
    upstreams[uslot].fd = ufd;
    upstreams[uslot].client_idx = client_idx;
    upstreams[uslot].cache_idx = -1;
    upstreams[uslot].stream_mode = false;
    upstreams[uslot].headers_complete = false;
    
    upstreams[uslot].req_len = snprintf(upstreams[uslot].request, 
                                        sizeof(upstreams[uslot].request),
                                        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", 
                                        path, host);
    
    pollfds[uslot].fd = ufd;
    pollfds[uslot].events = POLLOUT; 
    fd_register(ufd, uslot, FD_TYPE_UPSTREAM, fd_to_index);
    nfds++;
    
    int res = connect(ufd, (struct sockaddr*)&addr, sizeof(addr));
    if (res < 0 && errno != EINPROGRESS) {
        req_log(clients[client_idx].log_file, "Upstream connect failed immediately: %s", strerror(errno));
        safe_close(ufd);
        pollfds[uslot].fd = -1;
        nfds--;
        return -1;
    }
    
    req_log(clients[client_idx].log_file, "Upstream connecting: host=%s, port=%d, path=%s, uslot=%d", host, port, path, uslot);
    return uslot;
}

void upstream_disconnect(int ui) {
    if (ui < 0 || ui >= MAX_FDS) return;
    Upstream *up = &upstreams[ui];
    if (up->fd < 0) return;
    
    int ci_cache = up->cache_idx;
    int ci_client = up->client_idx;
    
    if (ci_client >= 0) {
        clients[ci_client].upstream_idx = -1;
    }
    
    if (ci_cache >= 0) {
        CacheEntry *ce = &caches[ci_cache];
        if (ce->state == CS_FETCHING) {
            ce->state = CS_COMPLETE;
            ce->content_len = ce->body_len; 
            cache_wake_waiters(ci_cache);
        }
    }
    
    safe_close(up->fd);
    fd_unregister(up->fd, fd_to_index);
    up->fd = -1;
    pollfds[ui].fd = -1;
    nfds--;
}

void upstream_handle_event(int ui, short revents) {
    if (ui < 0 || ui >= MAX_FDS) return;
    Upstream *up = &upstreams[ui];
    
    FILE *log_file = stderr;
    if (up->client_idx >= 0) log_file = clients[up->client_idx].log_file;
    else if (up->cache_idx >= 0 && caches[up->cache_idx].w_count > 0) {
        int wci = fd_to_idx_lookup(caches[up->cache_idx].waiters[0], FD_TYPE_CLIENT, fd_to_index);
        if (wci >= 0) log_file = clients[wci].log_file;
    }
    
    if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
        req_log(log_file, "Upstream poll error: revents=%d", revents);
        if (up->cache_idx >= 0) caches[up->cache_idx].state = CS_ERROR;
        upstream_disconnect(ui);
        return;
    }
    
    if (revents & POLLOUT && !up->headers_complete) {
        int soerr = 0;
        socklen_t soerr_len = sizeof(soerr);
        getsockopt(up->fd, SOL_SOCKET, SO_ERROR, &soerr, &soerr_len);
        
        if (soerr != 0) {
            req_log(log_file, "Upstream connect error: %s", strerror(soerr));
            if (up->cache_idx >= 0) caches[up->cache_idx].state = CS_ERROR;
            upstream_disconnect(ui);
            return;
        }
        
        ssize_t n = send(up->fd, up->request, up->req_len, 0);
        if (n > 0) {
            req_log(log_file, "Sent request to upstream (%zd bytes)", n);
            pollfds[ui].events = POLLIN; 
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            req_log(log_file, "Upstream send request error: %s", strerror(errno));
            upstream_disconnect(ui);
        }
        return;
    }
    
    if (revents & POLLIN) {
        ssize_t n = recv(up->fd, up->buf + up->len, BUF_SIZE - up->len - 1, 0);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            req_log(log_file, "Upstream recv error: %s", strerror(errno));
        } else if (n == 0) {
            req_log(log_file, "Upstream connection closed");
        }
        
        if (n <= 0) {
            upstream_disconnect(ui);
            return;
        }
        
        up->len += n;
        req_log(log_file, "Received %zd bytes from upstream", n);
        
        if (!up->headers_complete) {
            char *end = strstr(up->buf, "\r\n\r\n");
            if (end) {
                up->headers_complete = true;
                size_t hlen = (end - up->buf) + 4;
                up->temp_h_len = hlen;
                memcpy(up->temp_headers, up->buf, hlen);
                up->temp_headers[hlen] = '\0';
                
                memmove(up->buf, up->buf + hlen, up->len - hlen);
                up->len -= hlen;
                
                req_log(log_file, "Headers received (%zu bytes)", hlen);
                
                bool is_200 = is_status_200(up->temp_headers);
                size_t cl = 0;
                char *cl_hdr = strcasestr(up->temp_headers, "Content-Length:");
                if (cl_hdr) cl = atol(cl_hdr + 15);
                
                
                bool cacheable = is_200 && (cl > 0) && (cl <= MAX_CACHE_SIZE);
                
                int ci = up->client_idx;
                Client *cl_ptr = &clients[ci];
                int cidx = cl_ptr->cache_idx; 
                CacheEntry *ce = &caches[cidx];
                
                if (cacheable) {
                    if (!cache_ensure_space(cl, log_file)) {
                        req_log(log_file, "Cannot ensure space for %zu bytes, streaming instead", cl);
                        cacheable = false;
                    }
                }
                
                if (cacheable) {
                    req_log(log_file, "Caching response for %s, CL=%zu", cl_ptr->url, cl);
                    ce->state = CS_FETCHING;
                    ce->content_len = cl;
                    ce->body = malloc(cl);
                    if (!ce->body) {
                        req_log(log_file, "Failed to allocate cache body, streaming");
                        cacheable = false;
                    } else {
                        ce->body_len = 0;
                        ce->h_len = up->temp_h_len;
                        memcpy(ce->headers, up->temp_headers, up->temp_h_len);
                        ce->headers[ce->h_len] = '\0';
                        
                        memcpy(cl_ptr->send_buf, ce->headers, ce->h_len);
                        cl_ptr->send_len = ce->h_len;
                        cl_ptr->send_pos = 0;
                        cl_ptr->state = C_SEND_HEADERS;
                        cl_ptr->cache_offset = 0;
                        pollfds[ci].events |= POLLOUT;
                        
                        cache_add_waiter(cidx, cl_ptr->fd);
                        
                        up->cache_idx = cidx;
                        up->client_idx = -1;
                        cl_ptr->upstream_idx = -1;
                        
                        if (up->len > 0) {
                            size_t to_copy = up->len > cl ? cl : up->len;
                            memcpy(ce->body, up->buf, to_copy);
                            ce->body_len = to_copy;
                            current_cache_usage += to_copy;
                            memmove(up->buf, up->buf + to_copy, up->len - to_copy);
                            up->len -= to_copy;
                            
                            cache_wake_waiters(cidx);
                            
                            if (ce->body_len >= cl) {
                                ce->state = CS_COMPLETE;
                                cache_wake_waiters(cidx);
                                upstream_disconnect(ui);
                                return;
                            }
                        }
                    }
                } 
                
                if (!cacheable) {
                    req_log(log_file, "Streaming response for %s, CL=%zu", cl_ptr->url, cl);
                    ce->state = CS_EMPTY;
                    up->stream_mode = true;
                    cl_ptr->stream_mode = true;
                    cl_ptr->state = C_SEND_HEADERS;
                    
                    memcpy(cl_ptr->send_buf, up->temp_headers, up->temp_h_len);
                    cl_ptr->send_len = up->temp_h_len;
                    cl_ptr->send_pos = 0;
                    pollfds[ci].events |= POLLOUT;
                    
                    up->client_idx = ci;
                    cl_ptr->upstream_idx = ui;
                    
                    if (up->len > 0) {
                        client_append_stream(cl_ptr, up->buf, up->len);
                        up->len = 0;
                        pollfds[ci].events |= POLLOUT;
                    }
                }
            }
        } else {
            if (up->stream_mode && up->client_idx >= 0) {
                Client *cl_ptr = &clients[up->client_idx];
                client_append_stream(cl_ptr, up->buf, up->len);
                up->len = 0;
                pollfds[up->client_idx].events |= POLLOUT;
            } else if (up->cache_idx >= 0) {
                CacheEntry *ce = &caches[up->cache_idx];
                size_t space_left = ce->content_len - ce->body_len;
                size_t to_copy = up->len;
                if (to_copy > space_left) to_copy = space_left;
                
                memcpy(ce->body + ce->body_len, up->buf, to_copy);
                ce->body_len += to_copy;
                current_cache_usage += to_copy;
                
                memmove(up->buf, up->buf + to_copy, up->len - to_copy);
                up->len -= to_copy;
                
                cache_wake_waiters(up->cache_idx);
                if (ce->body_len >= ce->content_len) {
                    ce->state = CS_COMPLETE;
                    req_log(log_file, "Cache complete for %s", ce->url);
                    cache_wake_waiters(up->cache_idx);
                    upstream_disconnect(ui);
                }
            }
        }
    }
}