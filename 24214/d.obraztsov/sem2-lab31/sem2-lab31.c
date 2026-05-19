#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8080
#define MAX_SESSIONS 100
#define BUFFER_SIZE 16384
#define MAX_REQ_SIZE 8192

volatile sig_atomic_t working = 1;

typedef struct CacheNode {
    char *url;
    char *data;
    size_t length;
    int complete; // 1 если загрузка от сервера завершена
    struct CacheNode *next;
} CacheNode;

CacheNode *cache_head = NULL;

typedef struct {
    int active;
    int cli_fd;
    int srv_fd;

    char req_buf[MAX_REQ_SIZE];
    int req_len;
    int req_sent; // cколько байт запроса уже отправлено серверу
    int req_complete;

    char host[256];
    int port;
    char url[1024];

    CacheNode *cache_node;
    size_t sent_bytes; // cколько байт кэша отправлено клиенту

    int server_connected; // 0 - в процессе подключения, 1 - подключен
    int server_eof;     // 1 - сервер закрыл соединение
} Session;

Session sessions[MAX_SESSIONS];

void handle_sigint(int sig) {
    (void)sig;
    working = 0;
}

void free_cache() {
    CacheNode *current = cache_head;
    while (current != NULL) {
        CacheNode *next = current->next;
        free(current->url);
        free(current->data);
        free(current);
        current = next;
    }
    cache_head = NULL;
}

CacheNode* find_cache(const char *url) {
    for (CacheNode *n = cache_head; n != NULL; n = n->next) {
        if (strcmp(n->url, url) == 0) return n;
    }
    return NULL;
}

CacheNode* create_cache(const char *url) {
    CacheNode *node = calloc(1, sizeof(CacheNode));
    if (!node) return NULL;
   
    node->url = strdup(url);
    if (!node->url) {
        free(node);
        return NULL;
    }
    node->next = cache_head;
    cache_head = node;
    return node;
}

int append_cache(CacheNode *node, char *buf, size_t len) {
    char *tmp = realloc(node->data, node->length + len);
    if (!tmp) return -1;
   
    node->data = tmp;
    memcpy(node->data + node->length, buf, len);
    node->length += len;
    return 0;
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void close_session(Session *s) {
    if (s->cli_fd != -1) {
        close(s->cli_fd);
    }
    if (s->srv_fd != -1) {
        close(s->srv_fd);
    }
    memset(s, 0, sizeof(Session));
    s->cli_fd = -1;
    s->srv_fd = -1;
}

int connect_to_server(const char *host, int port) {
    struct hostent *he = gethostbyname(host);
    if (!he) return -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    set_nonblocking(sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr **)he->h_addr_list)[0];

    int res = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0 && errno != EINPROGRESS) {
        close(sock);
        return -1;
    }
    return sock;
}

void str_tolower(char *str) {
    for (; *str; ++str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str + 32;
        }
    }
}

int parse_http_request(Session *s) {
    char *headers_end = strstr(s->req_buf, "\r\n\r\n");
    if (!headers_end) return 0;

    s->req_complete = 1;

    char *search_ptr = s->req_buf;
    while ((search_ptr = strcasestr(search_ptr, "keep-alive")) != NULL) {
        if (search_ptr < headers_end) {
            memcpy(search_ptr, "close     ", 10);
        }
        search_ptr += 10;
    }

    char *host_ptr = strcasestr(s->req_buf, "Host: ");
    if (!host_ptr) return -1;
    host_ptr += 6;

    char *host_end = strstr(host_ptr, "\r\n");
    if (!host_end) return -1;

    int host_len = host_end - host_ptr;
    if (host_len >= (int)sizeof(s->host)) return -1;

    strncpy(s->host, host_ptr, host_len);
    s->host[host_len] = '\0';
    s->port = 80;

    char *colon = strchr(s->host, ':');
    if (colon) {
        *colon = '\0';
        s->port = atoi(colon + 1);
    }

    char *first_line_end = strstr(s->req_buf, "\r\n");
    if (first_line_end) {
        int line_len = first_line_end - s->req_buf;
        if (line_len < (int)sizeof(s->url)) {
            strncpy(s->url, s->req_buf, line_len);
            s->url[line_len] = '\0';
        }
    }

    return 1;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
   
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(listen_fd);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("Listen failed");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
   
    printf("Proxy started on port %d...\n", PORT);

    for (int i = 0; i < MAX_SESSIONS; i++) {
        memset(&sessions[i], 0, sizeof(Session));
        sessions[i].cli_fd = -1;
        sessions[i].srv_fd = -1;
    }

    struct pollfd fds[MAX_SESSIONS * 2 + 1];

    while (working) {
        for (int i = 0; i < MAX_SESSIONS * 2 + 1; i++) {
            fds[i].fd = -1;
            fds[i].events = 0;
            fds[i].revents = 0;
        }

        fds[0].fd = listen_fd;
        fds[0].events = POLLIN;

        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (!sessions[i].active) continue;
            Session *s = &sessions[i];

            int cli_idx = 1 + i;
            fds[cli_idx].fd = s->cli_fd;
            // всегда держим POLLIN на клиенте, чтобы вовремя поймать его отключение (recv == 0)
            fds[cli_idx].events |= POLLIN;
           
            if (s->req_complete && s->cache_node && s->sent_bytes < s->cache_node->length) {
                fds[cli_idx].events |= POLLOUT;
            }

            int srv_idx = 1 + MAX_SESSIONS + i;
            if (s->srv_fd != -1) {
                fds[srv_idx].fd = s->srv_fd;
                if (!s->server_connected || s->req_sent < s->req_len) {
                    fds[srv_idx].events |= POLLOUT; // ждем коннекта или досылаем куски запроса
                }
                if (s->server_connected && !s->server_eof) {
                    fds[srv_idx].events |= POLLIN;  
                }
            }
        }

        int poll_count = poll(fds, MAX_SESSIONS * 2 + 1, -1);
        if (poll_count < 0) {
            if (errno == EINTR) continue;
            perror("Poll error");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int new_fd = accept(listen_fd, NULL, NULL);
            if (new_fd >= 0) {
                set_nonblocking(new_fd);
                int slot_found = 0;
                for (int i = 0; i < MAX_SESSIONS; i++) {
                    if (!sessions[i].active) {
                        sessions[i].active = 1;
                        sessions[i].cli_fd = new_fd;
                        sessions[i].srv_fd = -1;
                        slot_found = 1;
                        break;
                    }
                }
                if (!slot_found) {
                    close(new_fd); // нет мест - закрываем сразу
                }
            }
        }

        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (!sessions[i].active) continue;
            Session *s = &sessions[i];

            int cli_idx = 1 + i;
            int srv_idx = 1 + MAX_SESSIONS + i;

            if (fds[cli_idx].revents & POLLIN) {
                if (!s->req_complete) {
                    int r = recv(s->cli_fd, s->req_buf + s->req_len, sizeof(s->req_buf) - s->req_len - 1, 0);
                    if (r <= 0) {
                        if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
                        close_session(s);
                        continue;
                    }
                    s->req_len += r;
                    s->req_buf[s->req_len] = '\0';

                    int parse_res = parse_http_request(s);
                    if (parse_res == 1) {
                        CacheNode *node = find_cache(s->url);
                        if (node) {
                            printf("[HIT] %s\n", s->url);
                            s->cache_node = node;
                        } else {
                            printf("[MISS] %s\n", s->url);
                            s->cache_node = create_cache(s->url);
                            if (!s->cache_node) {
                                close_session(s);
                                continue;
                            }
                            s->srv_fd = connect_to_server(s->host, s->port);
                            if (s->srv_fd < 0) close_session(s);
                        }
                    } else if (parse_res < 0 || s->req_len >= (int)sizeof(s->req_buf) - 1) {
                        close_session(s);
                        continue;
                    }
                } else {
                    // если запрос уже готов, но POLLIN сработал - клиент либо прислал лишнее либо закрылся
                    char dummy[64];
                    int r = recv(s->cli_fd, dummy, sizeof(dummy), 0);
                    if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                        close_session(s);
                        continue;
                    }
                }
            }

            if (s->srv_fd != -1 && (fds[srv_idx].revents & POLLOUT)) {
                if (!s->server_connected) {
                    int err = 0;
                    socklen_t len = sizeof(err);
                    getsockopt(s->srv_fd, SOL_SOCKET, SO_ERROR, &err, &len);
                    if (err != 0) {
                        close_session(s);
                        continue;
                    }
                    s->server_connected = 1;
                }

                if (s->server_connected && s->req_sent < s->req_len) {
                    int w = send(s->srv_fd, s->req_buf + s->req_sent, s->req_len - s->req_sent, 0);
                    if (w > 0) {
                        s->req_sent += w;
                    } else if (w < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        close_session(s);
                        continue;
                    }
                }
            }

            if (s->srv_fd != -1 && (fds[srv_idx].revents & POLLIN)) {
                char buf[BUFFER_SIZE];
                int r = recv(s->srv_fd, buf, sizeof(buf), 0);
                if (r > 0) {
                    if (append_cache(s->cache_node, buf, r) < 0) {
                        close_session(s);
                        continue;
                    }
                } else if (r == 0 || (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                    if (s->cache_node) s->cache_node->complete = 1;
                    s->server_eof = 1;
                    close(s->srv_fd);
                    s->srv_fd = -1;
                }
            }

            if (s->req_complete && s->cache_node && (fds[cli_idx].revents & POLLOUT)) {
                size_t to_send = s->cache_node->length - s->sent_bytes;
                if (to_send > 0) {
                    int w = send(s->cli_fd, s->cache_node->data + s->sent_bytes, to_send, 0);
                    if (w > 0) {
                        s->sent_bytes += w;
                    } else if (w < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        close_session(s);
                        continue;
                    }
                }
                if (s->cache_node->complete && s->sent_bytes == s->cache_node->length) {
                    close_session(s);
                }
            }
        }
    }

    printf("\nShutting down proxy...\n");
    for (int i = 0; i < MAX_SESSIONS; i++) {
        close_session(&sessions[i]);
    }
    close(listen_fd);
    free_cache();
    printf("Memory freed. Goodbye.\n");

    return 0;
}