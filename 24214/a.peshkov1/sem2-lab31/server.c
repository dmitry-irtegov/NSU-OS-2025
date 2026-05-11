#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include "http_parser.h"

#define MAX_CLIENTS 1024
#define INITIAL_BUFFER_SIZE 4096
#define PORT 8080
#define CACHE_TTL 300

#define MAX_CACHE_SIZE (512 * 1024 * 1024)
#define MAX_OBJECT_SIZE (50 * 1024 * 1024)

typedef enum {
    STATE_READ_REQUEST,
    STATE_DNS_RESOLVE,
    STATE_CONNECT_SERVER,
    STATE_SEND_SERVER,
    STATE_READ_SERVER,
    STATE_SEND_CLIENT,
    STATE_CLOSE
} State;

typedef struct CacheEntry {
    char *url;
    char *data;
    size_t size;
    time_t expires_at;
    struct CacheEntry *next;
} CacheEntry;

typedef struct {
    int client_fd;
    int server_fd;
    State state;
    char *buffer;
    size_t buf_capacity;
    size_t buf_len;
    size_t buf_sent;
    CacheEntry *cache_ptr;
    char url[256];
    HttpResponse http_res;
} Connection;

Connection connections[MAX_CLIENTS];
struct pollfd fds[MAX_CLIENTS * 2 + 1];
int nfds = 0;

CacheEntry *cache_head = NULL;
size_t current_cache_size = 0;

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int ensure_buffer_capacity(Connection *c, size_t additional_size) {
    if (c->buf_len + additional_size + 1 > c->buf_capacity) {
        size_t new_capacity = c->buf_capacity == 0 ? INITIAL_BUFFER_SIZE : c->buf_capacity * 2;
        while (c->buf_len + additional_size + 1 > new_capacity) {
            new_capacity *= 2;
        }

        if (new_capacity > MAX_OBJECT_SIZE) {
            fprintf(stderr, "Превышен максимальный размер буфера для соединения.\n");
            return 0;
        }

        char *new_buffer = realloc(c->buffer, new_capacity);
        if (!new_buffer) {
            perror("realloc failed");
            return 0;
        }
        c->buffer = new_buffer;
        c->buf_capacity = new_capacity;
    }
    return 1;
}

CacheEntry* find_in_cache(const char *url) {
    CacheEntry *curr = cache_head, *prev = NULL;
    time_t now = time(NULL);
    while (curr) {
        if (now > curr->expires_at) {
            if (prev) prev->next = curr->next;
            else cache_head = curr->next;
            CacheEntry *to_free = curr;
            curr = curr->next;

            current_cache_size -= to_free->size;
            free(to_free->url);
            free(to_free->data);
            free(to_free);
            continue;
        }
        if (strcmp(curr->url, url) == 0) return curr;
        prev = curr;
        curr = curr->next;
    }
    return NULL;
}

void add_to_cache(const char *url, const char *data, size_t size) {
    if (size == 0 || size > MAX_OBJECT_SIZE) return;

    while (current_cache_size + size > MAX_CACHE_SIZE && cache_head) {
        CacheEntry *curr = cache_head, *prev = NULL, *oldest = cache_head, *oldest_prev = NULL;
        while (curr) {
            if (curr->next == NULL) {
                oldest = curr;
                oldest_prev = prev;
            }
            prev = curr;
            curr = curr->next;
        }

        if (oldest_prev) {
            oldest_prev->next = oldest->next;
        } else {
            cache_head = oldest->next;
        }

        current_cache_size -= oldest->size;
        free(oldest->url);
        free(oldest->data);
        free(oldest);
    }

    CacheEntry *entry = malloc(sizeof(CacheEntry));
    if (!entry) return;

    entry->url = strdup(url);
    entry->data = malloc(size);
    if (!entry->data) {
        free(entry->url);
        free(entry);
        return;
    }

    memcpy(entry->data, data, size);
    entry->size = size;
    entry->expires_at = time(NULL) + CACHE_TTL;
    entry->next = cache_head;
    cache_head = entry;

    current_cache_size += size;
}

void init_conn(int i) {
    if (connections[i].client_fd != -1) close(connections[i].client_fd);
    if (connections[i].server_fd != -1) close(connections[i].server_fd);
    connections[i].client_fd = -1;
    connections[i].server_fd = -1;
    connections[i].state = STATE_CLOSE;

    if (connections[i].buffer) {
        free(connections[i].buffer);
        connections[i].buffer = NULL;
    }
    connections[i].buf_capacity = 0;
    connections[i].buf_len = 0;
    connections[i].buf_sent = 0;

    connections[i].cache_ptr = NULL;
    connections[i].url[0] = '\0';
    init_http_response(&connections[i].http_res);
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(listen_fd);

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_fd, 100);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        connections[i].buffer = NULL;
        init_conn(i);
    }

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    printf("Proxy server started on port %d\n", PORT);

    while (1) {
        int poll_timeout = -1;
        nfds = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (connections[i].client_fd != -1) {
                fds[nfds].fd = connections[i].client_fd;
                fds[nfds].events = 0;
                if (connections[i].state == STATE_READ_REQUEST) {
                    fds[nfds].events |= POLLIN;
                }
                if (connections[i].state == STATE_SEND_CLIENT) fds[nfds].events |= POLLOUT;
                nfds++;
            }
            if (connections[i].server_fd != -1) {
                fds[nfds].fd = connections[i].server_fd;
                fds[nfds].events = 0;
                if (connections[i].state == STATE_READ_SERVER) fds[nfds].events |= POLLIN;
                if (connections[i].state == STATE_CONNECT_SERVER || connections[i].state == STATE_SEND_SERVER)
                    fds[nfds].events |= POLLOUT;
                nfds++;
            }
        }

        poll(fds, nfds, poll_timeout);

        if (fds[0].revents & POLLIN) {
            int new_fd = accept(listen_fd, NULL, NULL);
            if (new_fd >= 0) {
                set_nonblocking(new_fd);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (connections[i].client_fd == -1 && connections[i].state == STATE_CLOSE) {
                        init_conn(i);
                        connections[i].client_fd = new_fd;
                        connections[i].state = STATE_READ_REQUEST;
                        ensure_buffer_capacity(&connections[i], INITIAL_BUFFER_SIZE);
                        printf("[Client %d] Connected\n", i);
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            Connection *c = &connections[i];
            if (c->state == STATE_CLOSE) continue;

            if (c->state == STATE_READ_REQUEST) {
                if (!ensure_buffer_capacity(c, 4096)) { init_conn(i); continue; }
                int bytes = recv(c->client_fd, c->buffer + c->buf_len, c->buf_capacity - c->buf_len - 1, 0);
                if (bytes <= 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        printf("[Client %d] recv failed or connection closed (bytes=%d, errno=%d)\n", i, bytes, errno);
                        init_conn(i);
                    }
                    continue;
                }
                c->buf_len += bytes;
                c->buffer[c->buf_len] = '\0';

                if (strstr(c->buffer, "\r\n\r\n")) {
                    printf("[Client %d] Full request received\n", i);

                    if (sscanf(c->buffer, "GET %255s", c->url) == 1) {
                        printf("[Client %d] URL: %s\n", i, c->url);
                        c->cache_ptr = find_in_cache(c->url);
                        if (c->cache_ptr) {
                            printf("[Client %d] Found in cache!\n", i);
                            c->state = STATE_SEND_CLIENT;
                            c->buf_sent = 0;
                        } else {
                            char *proxy_conn = strstr(c->buffer, "Proxy-Connection: Keep-Alive");
                            if (proxy_conn) {
                                memcpy(proxy_conn, "Connection: close           ", 28);
                            }
                            char *conn_ka = strstr(c->buffer, "Connection: keep-alive");
                            if (conn_ka) {
                                memcpy(conn_ka, "Connection: close     ", 22);
                            }

                            char host[256] = {0};
                            char *host_start = strstr(c->url, "://");
                            if (host_start) {
                                host_start += 3;
                            } else {
                                host_start = c->url;
                            }

                            int h_idx = 0;
                            while (host_start[h_idx] && host_start[h_idx] != '/' && host_start[h_idx] != ':' && h_idx < 255) {
                                host[h_idx] = host_start[h_idx];
                                h_idx++;
                            }
                            host[h_idx] = '\0';

                            if (strlen(host) == 0) {
                                char *host_header = strstr(c->buffer, "Host: ");
                                if (host_header) {
                                    host_header += 6;
                                    h_idx = 0;
                                    while (host_header[h_idx] && host_header[h_idx] != '\r' && host_header[h_idx] != ':' && h_idx < 255) {
                                        host[h_idx] = host_header[h_idx];
                                        h_idx++;
                                    }
                                    host[h_idx] = '\0';
                                }
                            }

                            printf("[Client %d] Host parsed: %s\n", i, host);

                            struct addrinfo hints;
                            struct addrinfo *result;
                            memset(&hints, 0, sizeof(hints));
                            hints.ai_family = AF_INET;
                            hints.ai_socktype = SOCK_STREAM;

                            int s = getaddrinfo(host, "80", &hints, &result);
                            if (s == 0 && result != NULL) {
                                c->server_fd = socket(AF_INET, SOCK_STREAM, 0);
                                set_nonblocking(c->server_fd);
                                struct sockaddr_in *remote = (struct sockaddr_in *)result->ai_addr;

                                int res = connect(c->server_fd, (struct sockaddr *)remote, sizeof(*remote));
                                if (res < 0 && errno != EINPROGRESS && errno != EWOULDBLOCK) {
                                     printf("[Client %d] Connect failed immediately\n", i);
                                     init_conn(i);
                                } else {
                                    c->state = STATE_CONNECT_SERVER;
                                    printf("[Client %d] Connecting to %s (fd=%d)\n", i, host, c->server_fd);
                                }
                                freeaddrinfo(result);
                            } else {
                                printf("[Client %d] getaddrinfo failed for %s\n", i, host);
                                init_conn(i);
                            }
                        }
                    } else {
                        printf("[Client %d] Failed to parse URL from GET\n", i);
                        init_conn(i);
                    }
                }
            }
            else if (c->state == STATE_CONNECT_SERVER || c->state == STATE_SEND_SERVER) {
                if (c->state == STATE_CONNECT_SERVER) {
                    int err = 0;
                    socklen_t len = sizeof(err);
                    if (getsockopt(c->server_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
                        printf("[Client %d] getsockopt failed\n", i);
                        init_conn(i); continue;
                    }
                    if (err != 0) {
                        if (err == EINPROGRESS || err == EWOULDBLOCK || err == EAGAIN) {
                             continue;
                        }
                        printf("[Client %d] Connection failed (err=%d)\n", i, err);
                        init_conn(i); continue;
                    }
                    printf("[Client %d] Connected to server, sending request\n", i);
                    c->state = STATE_SEND_SERVER;
                }

                if (c->state == STATE_SEND_SERVER) {
                    int bytes = send(c->server_fd, c->buffer, c->buf_len, 0);
                    if (bytes > 0) {
                        printf("[Client %d] Sent %d bytes to server\n", i, bytes);

                        if ((size_t)bytes < c->buf_len) {
                            memmove(c->buffer, c->buffer + bytes, c->buf_len - bytes);
                            c->buf_len -= bytes;
                        } else {
                            c->buf_len = 0;
                            c->state = STATE_READ_SERVER;
                        }
                    } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        printf("[Client %d] send to server failed (errno=%d)\n", i, errno);
                        init_conn(i);
                    }
                }
            }
            else if (c->state == STATE_READ_SERVER) {
                if (!ensure_buffer_capacity(c, 8192)) { init_conn(i); continue; }
                int bytes = recv(c->server_fd, c->buffer + c->buf_len, c->buf_capacity - c->buf_len - 1, 0);

                int response_complete = 0;

                if (bytes > 0) {
                    c->buf_len += bytes;
                    c->buffer[c->buf_len] = '\0';
                    parse_http_response(&c->http_res, c->buffer, c->buf_len);

                    if (c->http_res.headers_received) {
                        if (c->http_res.is_chunked) {
                            if (is_chunked_response_complete(c->buffer + c->http_res.body_start_offset, c->http_res.body_received)) {
                                response_complete = 1;
                            }
                        } else if (c->http_res.content_length != -1) {
                            if (c->http_res.body_received >= c->http_res.content_length) {
                                response_complete = 1;
                            }
                        }
                    }
                } else if (bytes == 0) {
                    response_complete = 1;
                    printf("[Client %d] Server connection closed gracefully.\n", i);
                } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    printf("[Client %d] recv from server failed (errno=%d)\n", i, errno);
                    init_conn(i);
                    continue;
                }

                if (response_complete) {
                    parse_http_response(&c->http_res, c->buffer, c->buf_len);
                    printf("[Client %d] Response complete. Status: %d\n", i, c->http_res.http_status);

                    if (c->http_res.http_status == 200) {
                         add_to_cache(c->url, c->buffer, c->buf_len);
                         printf("[Client %d] Saved to cache (size: %zu)\n", i, c->buf_len);
                    }

                    if (c->server_fd != -1) {
                         close(c->server_fd); c->server_fd = -1;
                    }

                    if (c->buf_len > 0) {
                        c->state = STATE_SEND_CLIENT; c->buf_sent = 0;
                    } else {
                        init_conn(i);
                    }
                }
            }
            else if (c->state == STATE_SEND_CLIENT) {
                char *data_to_send;
                size_t size_to_send;

                if (c->cache_ptr) {
                     data_to_send = c->cache_ptr->data;
                     size_to_send = c->cache_ptr->size;
                } else {
                     data_to_send = c->buffer;
                     size_to_send = c->buf_len;
                }

                if (size_to_send == 0) { init_conn(i); continue; }

                int bytes = send(c->client_fd, data_to_send + c->buf_sent, size_to_send - c->buf_sent, 0);
                if (bytes > 0) {
                    c->buf_sent += bytes;
                    if (c->buf_sent >= size_to_send) {
                        printf("[Client %d] Finished sending response to client\n", i);
                        init_conn(i);
                    }
                } else if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    printf("[Client %d] send to client failed (errno=%d)\n", i, errno);
                    init_conn(i);
                }
            }
        }
    }
    return 0;
}
