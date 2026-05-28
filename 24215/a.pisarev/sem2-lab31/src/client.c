#define _GNU_SOURCE
#include "client.h"
#include "cache.h"
#include "upstream.h"
#include "utils.h"

Client clients[MAX_FDS];
static int req_counter = 0;

void client_append_stream(Client *cl, const char *data, size_t len) {
    while (cl->stream_len + len > cl->stream_cap) {
        cl->stream_cap = cl->stream_cap ? cl->stream_cap * 2 : BUF_SIZE;
        cl->stream_buf = realloc(cl->stream_buf, cl->stream_cap);
        if (!cl->stream_buf) return;
    }
    memcpy(cl->stream_buf + cl->stream_len, data, len);
    cl->stream_len += len;
}

int client_accept(int listen_fd) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int cfd = accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
    if (cfd < 0) { 
        if (errno != EAGAIN && errno != EWOULDBLOCK) perror("accept"); 
        return -1;
    }
    set_nonblock(cfd);

    int slot = find_free_slot();
    if (slot < 0) { 
        safe_close(cfd); 
        fprintf(stderr, "[LOG] Max connections reached\n"); 
        return -1;
    }
    
    memset(&clients[slot], 0, sizeof(clients[slot]));
    clients[slot].fd = cfd;
    clients[slot].state = C_PARSE_REQ;
    clients[slot].cache_idx = -1;
    clients[slot].upstream_idx = -1;
    
    clients[slot].send_cap = BUF_SIZE;
    clients[slot].send_buf = malloc(clients[slot].send_cap);
    clients[slot].stream_cap = BUF_SIZE;
    clients[slot].stream_buf = malloc(clients[slot].stream_cap);
    
    unsigned long req_id = req_counter++;
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/req_%lu.log", LOG_DIR, req_id);
    clients[slot].log_file = fopen(log_path, "a");
    
    pollfds[slot].fd = cfd;
    pollfds[slot].events = POLLIN;
    fd_register(cfd, slot, FD_TYPE_CLIENT, fd_to_index);
    nfds++;
    
    req_log(clients[slot].log_file, "Client accepted: fd=%d, slot=%d", cfd, slot);
    return slot;
}

void client_disconnect(int ci) {
    if (ci < 0 || ci >= MAX_FDS) return;
    Client *cl = &clients[ci];
    if (cl->fd < 0) return;
    
    req_log(cl->log_file, "Client disconnecting: ci=%d, fd=%d", ci, cl->fd);
    
    if (cl->log_file) {
        fclose(cl->log_file);
        cl->log_file = NULL;
    }
    
    if (cl->upstream_idx >= 0) {
        upstream_disconnect(cl->upstream_idx);
        cl->upstream_idx = -1;
    }
    
    if (cl->cache_idx >= 0) {
        cache_remove_waiter(cl->cache_idx, cl->fd);
        cl->cache_idx = -1;
    }
    
    safe_close(cl->fd);
    fd_unregister(cl->fd, fd_to_index);
    cl->fd = -1;
    
    if (cl->send_buf) { free(cl->send_buf); cl->send_buf = NULL; }
    if (cl->stream_buf) { free(cl->stream_buf); cl->stream_buf = NULL; }
    
    pollfds[ci].fd = -1;
    nfds--;
}

static int client_parse_request(Client *cl) {
    char *end = strstr(cl->req_buf, "\r\n\r\n");
    if (!end) return 0; 
    
    *(end + 2) = '\0'; 
    
    if (sscanf(cl->req_buf, "%7s %1023s HTTP/", cl->method, cl->url) != 2) {
        snprintf(cl->send_buf, cl->send_cap, "HTTP/1.0 400 Bad Request\r\n\r\n");
        req_log(cl->log_file, "Bad request format");
        return -1;
    }
    
    if (strcmp(cl->method, "GET") != 0) {
        snprintf(cl->send_buf, cl->send_cap, "HTTP/1.0 501 Not Implemented\r\n\r\n");
        req_log(cl->log_file, "Method not implemented: %s", cl->method);
        return -1;
    }
    
    req_log(cl->log_file, "Request parsed: %s %s", cl->method, cl->url);
    return 1;
}

static int client_extract_host(Client *cl, char *host, size_t host_sz, int *port) {
    char *h_start = strcasestr(cl->req_buf, "Host:");
    if (!h_start) return -1;
    
    h_start += 5;
    while (*h_start == ' ') h_start++;
    
    char *h_end = strchr(h_start, '\r');
    if (!h_end) return -1;
    
    char *colon = memchr(h_start, ':', h_end - h_start);
    size_t len = colon ? (size_t)(colon - h_start) : (size_t)(h_end - h_start);
    
    if (len >= host_sz) len = host_sz - 1;
    strncpy(host, h_start, len);
    host[len] = '\0';
    
    if (colon) {
        *port = atoi(colon + 1);
        if (*port <= 0 || *port > 65535) *port = 80;
    } else {
        *port = 80;
    }
    
    return 0;
}

void client_handle_event(int ci, short revents) {
    Client *cl = &clients[ci];
    
    if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
        client_disconnect(ci);
        return;
    }
    
    if (cl->state == C_PARSE_REQ) {
        if (revents & POLLIN) {
            ssize_t n = recv(cl->fd, cl->req_buf + cl->req_len, BUF_SIZE - 1 - cl->req_len, 0);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                req_log(cl->log_file, "Client recv error: %s", strerror(errno));
                client_disconnect(ci); 
                return; 
            }
            if (n == 0) {
                req_log(cl->log_file, "Client closed connection during request");
                client_disconnect(ci);
                return;
            }
            cl->req_len += n;

            int parsed = client_parse_request(cl);
            if (parsed < 0) {
                cl->send_len = strlen(cl->send_buf);
                cl->send_pos = 0;
                cl->state = C_SEND_HEADERS;
                pollfds[ci].events |= POLLOUT;
                return;
            }
            if (parsed == 0) return;

            char host[256] = {0};
            int port = 80;
            char path[URL_SIZE] = "/";
            
            if (client_extract_host(cl, host, sizeof(host), &port) < 0) {
                snprintf(cl->send_buf, cl->send_cap, "HTTP/1.0 400 Bad Request\r\n\r\n");
                cl->send_len = strlen(cl->send_buf);
                cl->state = C_SEND_HEADERS;
                pollfds[ci].events |= POLLOUT;
                return;
            }
            parse_path(cl->url, path, sizeof(path));

            int cidx = cache_get_or_create(cl->url, cl->log_file);
            if (cidx < 0) {
                snprintf(cl->send_buf, cl->send_cap, "HTTP/1.0 500 Internal Server Error\r\n\r\n");
                cl->send_len = strlen(cl->send_buf);
                cl->state = C_SEND_HEADERS;
                pollfds[ci].events |= POLLOUT;
                return;
            }
            
            cl->cache_idx = cidx;
            CacheEntry *ce = &caches[cidx];
            
            if (ce->state == CS_FETCHING || ce->state == CS_COMPLETE) {
                req_log(cl->log_file, "Joining existing cache entry idx=%d (state=%d)", cidx, ce->state);
                memcpy(cl->send_buf, ce->headers, ce->h_len);
                cl->send_len = ce->h_len;
                cl->send_pos = 0;
                cl->state = C_SEND_HEADERS;
                cl->cache_offset = 0;
                cache_add_waiter(cidx, cl->fd);
                pollfds[ci].events |= POLLOUT;
                return;
            }
            
            if (ce->state == CS_EMPTY) {
                ce->state = CS_FETCHING; // Бронируем место
                int uslot = upstream_connect(host, port, path, ci);
                if (uslot < 0) {
                    ce->state = CS_EMPTY;
                    req_log(cl->log_file, "Upstream connect failed for %s", cl->url);
                    snprintf(cl->send_buf, cl->send_cap, "HTTP/1.0 502 Bad Gateway\r\n\r\n");
                    cl->send_len = strlen(cl->send_buf);
                    cl->send_pos = 0;
                    cl->state = C_SEND_HEADERS;
                    pollfds[ci].events |= POLLOUT;
                    return;
                }
                cl->upstream_idx = uslot;
                cl->state = C_WAIT_UPSTREAM;
                return;
            }
        }
    }
    else if (cl->state == C_SEND_HEADERS) {
        if (revents & POLLOUT) {
            ssize_t n = send(cl->fd, cl->send_buf + cl->send_pos, cl->send_len - cl->send_pos, 0);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                req_log(cl->log_file, "Client send error (headers): %s", strerror(errno));
                client_disconnect(ci);
                return;
            }
            if (n == 0) return;
            
            cl->send_pos += n;
            req_log(cl->log_file, "Sent %zd bytes of headers", n);
            
            if (cl->send_pos >= cl->send_len) {
                cl->state = C_SEND_BODY;
                if (!cl->stream_mode && cl->cache_idx >= 0) {
                    CacheEntry *ce = &caches[cl->cache_idx];
                    if (cl->cache_offset < ce->body_len || ce->state == CS_COMPLETE) {
                        pollfds[ci].events |= POLLOUT;
                    } else {
                        pollfds[ci].events &= ~POLLOUT;
                    }
                }
            }
        }
    }
    else if (cl->state == C_SEND_BODY) {
        if (revents & POLLOUT) {
            if (cl->stream_mode) {
                if (cl->stream_len > 0) {
                    ssize_t n = send(cl->fd, cl->stream_buf + cl->stream_pos, cl->stream_len - cl->stream_pos, 0);
                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                        req_log(cl->log_file, "Client send error (stream): %s", strerror(errno));
                        client_disconnect(ci);
                        return;
                    }
                    if (n == 0) return;
                    
                    cl->stream_pos += n;
                    req_log(cl->log_file, "Sent %zd bytes from stream_buf", n);
                    
                    if (cl->stream_pos >= cl->stream_len) {
                        cl->stream_len = 0;
                        cl->stream_pos = 0;
                    }
                }
                
                if (cl->stream_len == 0 && cl->upstream_idx < 0) {
                    req_log(cl->log_file, "Stream complete, disconnecting client");
                    client_disconnect(ci);
                    return;
                }
            } else {
                //Cache mode
                CacheEntry *ce = &caches[cl->cache_idx];
                if (cl->cache_offset < ce->body_len) {
                    size_t to_send = ce->body_len - cl->cache_offset;
                    if (to_send > BUF_SIZE) to_send = BUF_SIZE;
                    
                    ssize_t n = send(cl->fd, ce->body + cl->cache_offset, to_send, 0);
                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                        req_log(cl->log_file, "Client send error (cache): %s", strerror(errno));
                        client_disconnect(ci);
                        return;
                    }
                    if (n == 0) return;
                    
                    cl->cache_offset += n;
                    req_log(cl->log_file, "Sent %zd bytes from cache", n);
                }
                
                if (cl->cache_offset == ce->body_len && ce->state == CS_COMPLETE) {
                    req_log(cl->log_file, "Cache response complete, disconnecting client");
                    client_disconnect(ci);
                    return;
                }
                
                if (cl->cache_offset == ce->body_len && ce->state != CS_COMPLETE) {
                    pollfds[ci].events &= ~POLLOUT;
                }
            }
        }
    }
}