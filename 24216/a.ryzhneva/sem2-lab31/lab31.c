#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#include <errno.h>

#define BUF_SIZE 8192
#define MAX_SESSIONS 510
#define MAX_URI_LEN 2048
#define MAX_METHOD_LEN 16
#define MAX_VERSION_LEN 16
#define STATE_READ_REQUEST  1
#define STATE_CACHE_LOOKUP  2
#define STATE_FETCH_SERVER  3
#define STATE_SERVE_CACHE   4
#define STATE_SEND_REQUEST  5

volatile sig_atomic_t flag = 1;

typedef struct CacheEntry {
    char *uri;
    char *data;
    size_t size;
    size_t capacity;
    int is_complete;    
    int reference_count;
    struct CacheEntry *next;
} CacheEntry;

typedef struct {
    int active;
    int state;
    int fd_client;
    int fd_server;
    int connect_pending;
    int request_len;
    int request_sent;
    char request_buf[BUF_SIZE];
    char uri[MAX_URI_LEN];
    CacheEntry *cache_entry;
    size_t cache_offset;
    int pfd_client_idx;
    int pfd_server_idx;
} Session;

CacheEntry *cache_head = NULL;

CacheEntry* cache_lookup(const char *uri) {
    CacheEntry *curr = cache_head;
    while (curr != NULL) {
        if (strcmp(curr->uri, uri) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

CacheEntry* cache_add_entry(const char *uri) {
    CacheEntry *new_entry = (CacheEntry *)malloc(sizeof(CacheEntry));
    if (!new_entry) {
        return NULL;
    }

    new_entry->uri = strdup(uri);
    if (!new_entry->uri) {
        free(new_entry);
        return NULL;
    }

    new_entry->capacity = BUF_SIZE;
    new_entry->data = (char *)malloc(new_entry->capacity);
    if (!new_entry->data) {
        free(new_entry->uri);
        free(new_entry);
        return NULL;
    }

    new_entry->size = 0;
    new_entry->is_complete = 0;
    new_entry->reference_count = 1;

    new_entry->next = cache_head;
    cache_head = new_entry;

    return new_entry;
}

int cache_append_data(CacheEntry *entry, const char *new_data, size_t len) {
    if (entry->size + len > entry->capacity) {
        size_t new_capacity = entry->capacity * 2;
        while (entry->size + len > new_capacity) {
            new_capacity *= 2;
        }
        char *temp = (char *)realloc(entry->data, new_capacity);
        if (!temp) {
            return -1;
        }
        entry->data = temp;
        entry->capacity = new_capacity;
    }
    
    memcpy(entry->data + entry->size, new_data, len);
    entry->size += len;
    return 0;
}

void cache_release_entry(CacheEntry *entry) {
    if (!entry) {
        return;
    }
    entry->reference_count--;
}

void cache_free_all() {
    CacheEntry *curr = cache_head;
    while (curr != NULL) {
        CacheEntry *next = curr->next;
        free(curr->uri);
        free(curr->data);
        free(curr);
        curr = next;
    }
    cache_head = NULL;
}

static const char *default_target_host = NULL;
static const char *default_target_port = NULL;
Session sessions[MAX_SESSIONS];

void sig_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        flag = 0;
    }
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return;
    }
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

void close_session(Session *s) {
    if (!s->active) {
        return;
    }

    shutdown(s->fd_client, SHUT_RDWR);
    close(s->fd_client);

    if (s->fd_server >= 0) {
        shutdown(s->fd_server, SHUT_RDWR);
        close(s->fd_server);
    }

    if (s->cache_entry != NULL) {
        cache_release_entry(s->cache_entry);
        s->cache_entry = NULL;
    }

    s->active = 0;
}

int parse_http_request(Session *s) {
    if (strstr(s->request_buf, "\r\n\r\n") == NULL) {
        return 0;
    }

    char method[MAX_METHOD_LEN];
    char version[MAX_VERSION_LEN];

    int matched = sscanf(s->request_buf, "%15s %2047s %15s", method, s->uri, version);

    if (matched != 3) {
        fprintf(stderr, "Invalid HTTP request format.\n");
        return -1;
    }

    if (strncmp(method, "GET", 3) != 0) {
        fprintf(stderr, "Unsupported HTTP method: %s\n", method);
        return -1;
    }

    return 1;
}

int parse_target_from_uri(const char *uri, char *host, size_t host_len, char *port, size_t port_len) {
    const char *scheme = "http://";
    size_t scheme_len = strlen(scheme);

    if (strncmp(uri, scheme, scheme_len) != 0) {
        return 0;
    }

    const char *host_start = uri + scheme_len;
    const char *path_start = strchr(host_start, '/');
    const char *host_end = path_start ? path_start : uri + strlen(uri);
    const char *colon = memchr(host_start, ':', (size_t)(host_end - host_start));

    if (host_start == host_end) {
        return -1;
    }

    if (colon) {
        size_t host_part_len = (size_t)(colon - host_start);
        size_t port_part_len = (size_t)(host_end - colon - 1);

        if (host_part_len == 0 || port_part_len == 0) {
            return -1;
        }

        if (host_part_len >= host_len || port_part_len >= port_len) {
            return -1;
        }

        memcpy(host, host_start, host_part_len);
        host[host_part_len] = '\0';

        memcpy(port, colon + 1, port_part_len);
        port[port_part_len] = '\0';
    } else {
        size_t host_part_len = (size_t)(host_end - host_start);
        if (host_part_len == 0 || host_part_len >= host_len) {
            return -1;
        }

        memcpy(host, host_start, host_part_len);
        host[host_part_len] = '\0';

        if (snprintf(port, port_len, "%s", "80") >= (int)port_len) {
            return -1;
        }
    }

    return 1;
}

int parse_host_header(const char *request, char *host, size_t host_len, char *port, size_t port_len) {
    const char *p = request;
    while ((p = strstr(p, "\r\n")) != NULL) {
        p += 2;
        if (strncasecmp(p, "Host:", 5) == 0) {
            p += 5;
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            const char *line_end = strstr(p, "\r\n");
            if (!line_end) return -1;

            const char *colon = memchr(p, ':', (size_t)(line_end - p));
            if (colon) {
                size_t host_len_part = (size_t)(colon - p);
                size_t port_len_part = (size_t)(line_end - colon - 1);
                if (host_len_part == 0 || port_len_part == 0) return -1;
                if (host_len_part >= host_len || port_len_part >= port_len) return -1;

                memcpy(host, p, host_len_part);
                host[host_len_part] = '\0';
                memcpy(port, colon + 1, port_len_part);
                port[port_len_part] = '\0';
            } else {
                size_t host_len_part = (size_t)(line_end - p);
                if (host_len_part == 0 || host_len_part >= host_len) return -1;

                memcpy(host, p, host_len_part);
                host[host_len_part] = '\0';
                if (snprintf(port, port_len, "%s", "80") >= (int)port_len) return -1;
            }
            return 1;
        }
    }
    return 0;
}

struct addrinfo *resolve_target(const char *target_host, const char *target_port) {
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(target_host, target_port, &hints, &result) != 0) {
        return NULL;
    }

    return result;
}

int init_listen_socket(const char *port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Ошибка создания слушающего сокета");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(port));

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("Ошибка listen");
        exit(EXIT_FAILURE);
    }

    if (fcntl(listen_fd, F_SETFL, O_NONBLOCK)) {
        perror("Ошибка fcntl");
        exit(EXIT_FAILURE);
    }
    
    return listen_fd;
}

void init_sessions() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        sessions[i].active = 0;
    }
}

int build_poll_array(struct pollfd *pfds, int listen_fd) {
    int nfds = 0;
    int free_slots = 0;

    pfds[nfds].fd = listen_fd;
    pfds[nfds].events = 0;
    pfds[nfds].revents = 0;
    nfds++;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions[i].active) {
            free_slots++;
            continue;
        }

        Session *s = &sessions[i];

        int client_idx = nfds++;
        s->pfd_client_idx = client_idx;
        pfds[client_idx].fd = s->fd_client;
        pfds[client_idx].events = 0;
        pfds[client_idx].revents = 0;

        if (s->state == STATE_READ_REQUEST) {
            pfds[client_idx].events |= POLLIN;
        } 
        else if ((s->state == STATE_SERVE_CACHE || s->state == STATE_FETCH_SERVER) && s->cache_entry != NULL) {
            if (s->cache_offset < s->cache_entry->size) {
                pfds[client_idx].events |= POLLOUT;
            }
        }

        if ((s->state == STATE_FETCH_SERVER || s->state == STATE_SEND_REQUEST) && s->fd_server >= 0) {
            int server_idx = nfds++;
            s->pfd_server_idx = server_idx;
            pfds[server_idx].fd = s->fd_server;
            pfds[server_idx].events = 0;
            pfds[server_idx].revents = 0;

            if (s->state == STATE_FETCH_SERVER) {
                pfds[server_idx].events |= POLLIN;
            } else if (s->state == STATE_SEND_REQUEST) {
                pfds[server_idx].events |= POLLOUT;
            }
        } else {
            s->pfd_server_idx = -1;
        }
    }

    if (free_slots > 0) {
        pfds[0].events |= POLLIN;
    }

    return nfds;
}

void handle_session_io(Session *s, struct pollfd *pfds) {
    int client_idx = s->pfd_client_idx;
    int server_idx = s->pfd_server_idx;

    if ((client_idx >= 0 && (pfds[client_idx].revents & (POLLERR | POLLNVAL | POLLHUP))) || 
        (server_idx >= 0 && (pfds[server_idx].revents & (POLLERR | POLLNVAL | POLLHUP)))) {
        close_session(s);
        return;
    }

    switch (s->state) {
        case STATE_READ_REQUEST:
            if (client_idx >= 0 && (pfds[client_idx].revents & POLLIN)) {

                if (s->request_len >= BUF_SIZE - 1) {
                    close_session(s);
                    break;
                }

                int bytes_read = recv(s->fd_client, s->request_buf + s->request_len, BUF_SIZE - 1 - s->request_len, 0);

                if (bytes_read > 0) {
                    s->request_len += bytes_read;
                    s->request_buf[s->request_len] = '\0';

                    int parse_code = parse_http_request(s);

                    if (parse_code == 1) {
                        char target_host[256];
                        char target_port[16];
                        int parsed = parse_target_from_uri(s->uri, target_host, sizeof(target_host), target_port, sizeof(target_port));
                        int uri_is_absolute = (parsed == 1);
                        if (parsed == -1) {
                            close_session(s);
                            break;
                        } else if (parsed == 0) {
                            parsed = parse_host_header(s->request_buf, target_host, sizeof(target_host), target_port, sizeof(target_port));
                            if (parsed == -1) {
                                close_session(s);
                                break;
                            }
                        }

                        const char *host_to_use = (parsed == 1) ? target_host : default_target_host;
                        const char *port_to_use = (parsed == 1) ? target_port : default_target_port;
                        if (!host_to_use || !port_to_use) {
                            close_session(s);
                            break;
                        }

                        char cache_key[MAX_URI_LEN + 300];
                        if (uri_is_absolute) {
                            if (snprintf(cache_key, sizeof(cache_key), "%s", s->uri) >= (int)sizeof(cache_key)) {
                                close_session(s);
                                break;
                            }
                        } else {
                            const char *path_prefix = (s->uri[0] == '/') ? "" : "/";
                            if (snprintf(cache_key, sizeof(cache_key), "http://%s:%s%s%s", host_to_use, port_to_use, path_prefix, s->uri) >= (int)sizeof(cache_key)) {
                                close_session(s);
                                break;
                            }
                        }

                        CacheEntry *found = cache_lookup(cache_key);
                        if (found != NULL) {
                            s->cache_entry = found;
                            found->reference_count++;
                            s->cache_offset = 0;
                            s->state = STATE_SERVE_CACHE;
                            break;
                        }

                        s->cache_entry = cache_add_entry(cache_key);
                        s->cache_offset = 0;

                        if (!s->cache_entry) {
                            close_session(s);
                            break;
                        }

                        struct addrinfo *target_addrinfo = resolve_target(host_to_use, port_to_use);
                        if (!target_addrinfo) {
                            close_session(s);
                            break;
                        }

                        s->fd_server = socket(target_addrinfo->ai_family, target_addrinfo->ai_socktype, target_addrinfo->ai_protocol);
                        if (s->fd_server < 0) {
                            freeaddrinfo(target_addrinfo);
                            close_session(s);
                            break;
                        }

                        set_nonblock(s->fd_server);

                        if (connect(s->fd_server, target_addrinfo->ai_addr, target_addrinfo->ai_addrlen) < 0) {
                            if (errno != EINPROGRESS) {
                                freeaddrinfo(target_addrinfo);
                                close_session(s);
                                break;
                            }
                            s->connect_pending = 1;
                        } else {
                            s->connect_pending = 0;
                            set_blocking(s->fd_server);
                        }

                        freeaddrinfo(target_addrinfo);

                        s->request_sent = 0;
                        s->state = STATE_SEND_REQUEST;
                    } else if (parse_code == -1) {
                        close_session(s);
                    }
                } else if (bytes_read == 0) {
                    close_session(s);
                } else {
                    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                        close_session(s);
                    }
                }
            }
            break;
        case STATE_SEND_REQUEST: {
            if (server_idx >= 0 && (pfds[server_idx].revents & POLLOUT)) {
                if (s->connect_pending) {
                    int so_error = 0;
                    socklen_t so_error_len = sizeof(so_error);
                    if (getsockopt(s->fd_server, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len) < 0 || so_error != 0) {
                        close_session(s);
                        break;
                    }
                    set_blocking(s->fd_server);
                    s->connect_pending = 0;
                }

                int sent = send(s->fd_server, 
                                s->request_buf + s->request_sent, 
                                s->request_len - s->request_sent, 
                                0);
                
                if (sent > 0) {
                    s->request_sent += sent;
                    if (s->request_sent == s->request_len) {
                        s->state = STATE_FETCH_SERVER;
                    }
                } else if (sent < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("Ошибка отправки запроса серверу");
                        close_session(s);
                    }
                }
            }
            break;
        }

        case STATE_FETCH_SERVER: {
            if (server_idx >= 0 && (pfds[server_idx].revents & POLLIN)) {
                char temp_buf[BUF_SIZE];
                int bytes_read = recv(s->fd_server, temp_buf, sizeof(temp_buf), 0);

                if (bytes_read > 0) {
                    if (cache_append_data(s->cache_entry, temp_buf, bytes_read) < 0) {
                        close_session(s);
                        break;
                    }
                } else if (bytes_read == 0) {
                    s->cache_entry->is_complete = 1;
                    
                    close(s->fd_server);
                    s->fd_server = -1;
                    s->state = STATE_SERVE_CACHE;
                } else {
                    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                        close_session(s);
                    }
                }
            }
            if (client_idx >= 0 && (pfds[client_idx].revents & POLLOUT)) {
                size_t available_bytes = s->cache_entry->size - s->cache_offset;
                
                if (available_bytes > 0) {
                    int bytes_sent = send(s->fd_client, 
                                          s->cache_entry->data + s->cache_offset, 
                                          available_bytes, 
                                          0);
                    
                    if (bytes_sent > 0) {
                        s->cache_offset += bytes_sent;
                    } else if (bytes_sent < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                            close_session(s);
                            break;
                        }
                    }
                }
            }
            break;
        }

        case STATE_SERVE_CACHE: {
            if (client_idx >= 0 && (pfds[client_idx].revents & POLLOUT)) {
                size_t available_bytes = s->cache_entry->size - s->cache_offset;
                
                if (available_bytes > 0) {
                    int bytes_sent = send(s->fd_client, 
                                          s->cache_entry->data + s->cache_offset, 
                                          available_bytes, 
                                          0);
                    
                    if (bytes_sent > 0) {
                        s->cache_offset += bytes_sent;
                    } else if (bytes_sent < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                            close_session(s);
                            break;
                        }
                    }
                }
                
                if (s->cache_offset == s->cache_entry->size && s->cache_entry->is_complete == 1) {
                    close_session(s);
                }
            }
            break;
        }
    }
}

void handle_new_connection(int listen_fd) {
    int new_client_fd = accept(listen_fd, NULL, NULL);
    if (new_client_fd < 0) {
        return;
    }

    int free_idx = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions[i].active) {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1) {
        close(new_client_fd);
        return;
    }

    set_nonblock(new_client_fd);

    sessions[free_idx].active = 1;
    sessions[free_idx].state = STATE_READ_REQUEST;
    sessions[free_idx].fd_client = new_client_fd;
    sessions[free_idx].fd_server = -1;
    sessions[free_idx].request_len = 0;
    sessions[free_idx].request_sent = 0;
    sessions[free_idx].connect_pending = 0;
    sessions[free_idx].uri[0] = '\0';
    sessions[free_idx].cache_entry = NULL;
    sessions[free_idx].cache_offset = 0;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Invalid input. Incorrect count of argument.\n");
        return EXIT_FAILURE;
    }

    const char *listen_port = argv[1];
    const char *target_host = argv[2];
    const char *target_port = argv[3];

    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    default_target_host = target_host;
    default_target_port = target_port;
    int listen_fd = init_listen_socket(listen_port);
    init_sessions();

    struct pollfd pfds[MAX_SESSIONS * 2 + 1];

    while(flag) {
        int nfds = build_poll_array(pfds, listen_fd);

        if (poll(pfds, nfds, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Ошибка poll");
            break;
        }

        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (sessions[i].active) {
                handle_session_io(&sessions[i], pfds);
            }
        }

        if (pfds[0].revents & POLLIN) {
            handle_new_connection(listen_fd);
        }
    }

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].active) {
            close_session(&sessions[i]);
        }
    }

    close(listen_fd);
    cache_free_all();

    return 0;
} 