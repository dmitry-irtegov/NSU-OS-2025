#include "client.h"
#include "cache.h"
#include "upstream.h"
#include "utils.h"

Client clients[MAX_FDS];

int client_accept(int listen_fd) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int cfd = accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
    if (cfd < 0) { 
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
        perror("accept"); 
        return -1;
    }
    
    set_nonblock(cfd);
    int slot = find_free_slot();
    if (slot < 0) { 
        safe_close(cfd); 
        fprintf(stderr, "Max connections reached\n"); 
        return -1;
    }
    
    memset(&clients[slot], 0, sizeof(clients[slot]));
    clients[slot].fd = cfd;
    clients[slot].state = C_PARSE_REQ;
    clients[slot].cache_idx = -1;
    pollfds[slot].fd = cfd;
    pollfds[slot].events = POLLIN;
    fd_register(cfd, slot, FD_TYPE_CLIENT, fd_to_index);
    nfds++;
    
    return slot;
}

void client_disconnect(int ci) {
    if (ci < 0 || ci >= MAX_FDS) return;
    
    Client *cl = &clients[ci];
    
    if (cl->cache_idx >= 0) {
        CacheEntry *ce = &caches[cl->cache_idx];
        if (ce->state == CS_FETCHING) {
            cache_remove_waiter(cl->cache_idx, cl->fd);
        }
    }
    
    safe_close(cl->fd);
    fd_unregister(cl->fd, fd_to_index);
    cl->fd = -1;
    pollfds[ci].fd = -1;
    nfds--;
}

static int client_parse_request(Client *cl) {
    char *end = strstr(cl->req_buf, "\r\n\r\n");
    if (!end) return 0; 
    
    *(end + 2) = '\0';
    
    if (sscanf(cl->req_buf, "%7s %1023s HTTP/", cl->method, cl->url) != 2) {
        snprintf(cl->send_buf, BUF_SIZE, "HTTP/1.0 400 Bad Request\r\n\r\n");
        return -1;
    }
    
    if (strcmp(cl->method, "GET") != 0) {
        snprintf(cl->send_buf, BUF_SIZE, "HTTP/1.0 501 Not Implemented\r\n\r\n");
        return -1;
    }
    
    return 1;
}

int is_status_200(const char *headers) {
    return (strstr(headers, "HTTP/1.") && 
            (strstr(headers, " 200 ") || strstr(headers, " 200\r\n")));
}

static int client_extract_host(Client *cl, char *host, size_t host_sz, int *port) {
    char *h_start = strstr(cl->req_buf, "Host:");
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
    
    if (revents & POLLIN && cl->state == C_PARSE_REQ) {
        ssize_t n = recv(cl->fd, cl->req_buf + cl->req_len,
                        BUF_SIZE - 1 - cl->req_len, 0);
        if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) { 
            client_disconnect(ci); 
            return; 
        }
        if (n > 0) cl->req_len += n;

        int parsed = client_parse_request(cl);
        if (parsed < 0) {
            cl->send_len = strlen(cl->send_buf);
            cl->send_pos = 0;
            cl->state = C_SEND_RESP;
            pollfds[ci].events |= POLLOUT;
            return;
        }
        if (parsed == 0) return;  

        char host[256] = {0};
        int port = 80;
        char path[URL_SIZE] = "/";
        
        if (client_extract_host(cl, host, sizeof(host), &port) < 0) {
            snprintf(cl->send_buf, BUF_SIZE, "HTTP/1.0 400 Bad Request\r\n\r\n");
            cl->send_len = strlen(cl->send_buf);
            cl->state = C_SEND_RESP;
            pollfds[ci].events |= POLLOUT;
            return;
        }
        parse_path(cl->url, path, sizeof(path));

        int cidx = cache_get_or_create(cl->url);
        cl->cache_idx = cidx;
        
        if (cidx < 0) {
            snprintf(cl->send_buf, BUF_SIZE, "HTTP/1.0 507 Insufficient Storage\r\n\r\n");
            cl->send_len = strlen(cl->send_buf);
            cl->state = C_SEND_RESP;
            pollfds[ci].events |= POLLOUT;
            return;
        }
        
        CacheEntry *ce = &caches[cidx];
        
        if (ce->state == CS_COMPLETE) {
            size_t hlen = ce->h_len;
            if (hlen >= 4 && memcmp(ce->headers + hlen - 4, "\r\n\r\n", 4) != 0) {
                snprintf(cl->send_buf, BUF_SIZE, "%s\r\n", ce->headers);
            } else {
                snprintf(cl->send_buf, BUF_SIZE, "%s", ce->headers);
            }
            cl->send_len = strlen(cl->send_buf);
            cl->send_pos = 0;
            cl->cache_offset = 0;
            cl->state = C_SEND_RESP;
            pollfds[ci].events |= POLLOUT;
            return;
        }
        
        if (ce->state == CS_EMPTY) {
            CacheState expected = CS_EMPTY;
            int success = atomic_compare_exchange_strong(
                &ce->state, &expected, CS_FETCHING);
            
            if (success) {

                ce->w_count = 0;
                if (upstream_connect(host, port, path, cidx) < 0) {
                    ce->state = CS_ERROR;
                    cache_notify_waiters(cidx);
                }
            }

        }
        

        cache_add_waiter(cidx, cl->fd);
        cl->state = C_WAIT_CACHE;
        pollfds[ci].events = POLLIN;
        return;
    }
    

    if (revents & POLLOUT && cl->state == C_SEND_RESP) {
        if (cl->cache_idx >= 0) {
            CacheEntry *ce = &caches[cl->cache_idx];
            

            if (cl->send_pos < cl->send_len) {
                ssize_t n = send(cl->fd, cl->send_buf + cl->send_pos,
                                cl->send_len - cl->send_pos, 0);
                if (n > 0) {
                    cl->send_pos += n;
                    if (cl->send_pos < cl->send_len) return;
                } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    client_disconnect(ci);
                    return;
                }
            }
            

            size_t avail = ce->body_len - cl->cache_offset;
            if (avail > 0) {
                size_t to_send = (avail > BUF_SIZE) ? BUF_SIZE : avail;
                ssize_t n = send(cl->fd, ce->body + cl->cache_offset, to_send, 0);
                if (n > 0) {
                    cl->cache_offset += n;
                    if (cl->cache_offset < ce->body_len) {
                        if (ce->state == CS_FETCHING) {
                            pollfds[ci].events = POLLIN | POLLOUT;
                        }
                        return;
                    }
                } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    client_disconnect(ci);
                    return;
                }
            }
            
            if (ce->state == CS_FETCHING && avail == 0) {
                cl->state = C_WAIT_CACHE;
                pollfds[ci].events = POLLIN;
                return;
            }
        }
        
        client_disconnect(ci);
    }
}