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
    int request_len;
    int request_sent;
    char request_buf[BUF_SIZE];
    char uri[2048];
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

    new_entry->capacity = 8192;
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
    if (!entry) return;
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

struct addrinfo *target_addrinfo = NULL;
Session sessions[MAX_SESSIONS];

void sig_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        flag = 0;
    }
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
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

    char method[16];
    char version[16];

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

void resolve_target(const char *target_host, const char *target_port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(target_host, target_port, &hints, &target_addrinfo) != 0) {
        perror("Ошибка getaddrinfo для узла N");
        exit(EXIT_FAILURE);
    }
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
                        CacheEntry *found = cache_lookup(s->uri);

                        if (found != NULL) {
                            s->cache_entry = found;
                            found->reference_count++;
                            s->cache_offset = 0;
                            s->state = STATE_SERVE_CACHE;
                        } else {
                            s->cache_entry = cache_add_entry(s->uri);
                            s->cache_offset = 0;

                            if (!s->cache_entry) {
                                close_session(s);
                                break;
                            }

                            s->fd_server = socket(target_addrinfo->ai_family, target_addrinfo->ai_socktype, target_addrinfo->ai_protocol);
                            if (s->fd_server < 0) {
                                close_session(s);
                                break;
                            }

                            set_nonblock(s->fd_server);

                            if (connect(s->fd_server, target_addrinfo->ai_addr, target_addrinfo->ai_addrlen) < 0) {
                                if (errno != EINPROGRESS) {
                                    close_session(s);
                                    break;
                                }
                            }

                            s->request_sent = 0;
                            s->state = STATE_SEND_REQUEST;
                        }
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
                char temp_buf[8192];
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

    resolve_target(target_host, target_port);
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

    if (target_addrinfo) {
        freeaddrinfo(target_addrinfo);
    }
    
    close(listen_fd);
    cache_free_all();

    return 0;
} 