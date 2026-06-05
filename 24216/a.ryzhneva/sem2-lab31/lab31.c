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
#include <strings.h>
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
    int is_failed;    
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
    char version[MAX_VERSION_LEN];
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
    new_entry->is_failed = 0;
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

static int is_valid_port_string(const char *port) {
    if (!port || *port == '\0') {
        return 0;
    }
    int value = 0;
    for (const char *p = port; *p; ++p) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
        value = value * 10 + (*p - '0');
        if (value > 65535) {
            return 0;
        }
    }
    return value > 0;
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

static void send_http_error(Session *s, const char *status_line) {
    if (!s || !status_line) {
        return;
    }
    char response[128];
    int len = snprintf(response, sizeof(response), "HTTP/1.0 %s\r\n\r\n", status_line);
    if (len <= 0 || len >= (int)sizeof(response)) {
        return;
    }
    send(s->fd_client, response, (size_t)len, 0);
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
    char *end_of_headers = strstr(s->request_buf, "\r\n\r\n");
    if (end_of_headers == NULL) {
        return 0;
    }

    char method[MAX_METHOD_LEN];
    char version[MAX_VERSION_LEN];

    int matched = sscanf(s->request_buf, "%15s %2047s %15s", method, s->uri, version);

    if (matched != 3) {
        fprintf(stderr, "Invalid HTTP request format.\n");
        return -1;
    }

    if (snprintf(s->version, sizeof(s->version), "%s", version) >= (int)sizeof(s->version)) {
        fprintf(stderr, "Invalid HTTP version length.\n");
        return -1;
    }

    if (strncmp(method, "GET", 3) != 0) {
        fprintf(stderr, "Unsupported HTTP method: %s\n", method);
        return -1;
    }

    if (strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0) {
        fprintf(stderr, "Invalid HTTP version: %s\n", version);
        return -1;
    }

    if (strncmp(s->uri, "http://", 7) != 0) {
        fprintf(stderr, "Invalid request target: %s\n", s->uri);
        return -1;
    }

    char *version_ptr = strstr(s->request_buf, "HTTP/1.");
    if (version_ptr && (version_ptr < end_of_headers)) {
        memcpy(version_ptr, "HTTP/1.0", 8);
    }

    const char *conn_close = "\r\nConnection: close";
    size_t conn_len = strlen(conn_close);
    size_t headers_len = end_of_headers - s->request_buf;

    if (headers_len + conn_len + 4 < BUF_SIZE) {
        memmove(end_of_headers + conn_len, end_of_headers, (s->request_len - headers_len) + 1);
        memcpy(end_of_headers, conn_close, conn_len);
        s->request_len += conn_len;
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
        if (!is_valid_port_string(port)) {
            return -1;
        }
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

int replace_request_target(Session *s, const char *new_target) {
    char *line_end = strstr(s->request_buf, "\r\n");
    if (!line_end) {
        return -1;
    }

    char *first_space = memchr(s->request_buf, ' ', (size_t)(line_end - s->request_buf));
    if (!first_space) {
        return -1;
    }

    char *second_space = memchr(first_space + 1, ' ', (size_t)(line_end - (first_space + 1)));
    if (!second_space) {
        return -1;
    }

    size_t old_len = (size_t)(second_space - (first_space + 1));
    size_t new_len = strlen(new_target);
    size_t new_total = (size_t)s->request_len - old_len + new_len;

    if (new_total >= BUF_SIZE) {
        return -1;
    }

    memmove(first_space + 1 + new_len, second_space,
            (size_t)s->request_len - (size_t)(second_space - s->request_buf) + 1);
    memcpy(first_space + 1, new_target, new_len);
    s->request_len = (int)new_total;
    return 0;
}

int set_host_header(Session *s, const char *host, const char *port) {
    char host_value[300];
    if (!host || !port) {
        return -1;
    }

    if (strcmp(port, "80") == 0) {
        if (snprintf(host_value, sizeof(host_value), "%s", host) >= (int)sizeof(host_value)) {
            return -1;
        }
    } else {
        if (snprintf(host_value, sizeof(host_value), "%s:%s", host, port) >= (int)sizeof(host_value)) {
            return -1;
        }
    }

    char *headers_end = strstr(s->request_buf, "\r\n\r\n");
    if (!headers_end) {
        return -1;
    }

    char *line = strstr(s->request_buf, "\r\n");
    while (line && line < headers_end) {
        line += 2;
        if (line >= headers_end) {
            break;
        }

        if (strncasecmp(line, "Host:", 5) == 0) {
            char *value_start = line + 5;
            while (*value_start == ' ' || *value_start == '\t') {
                value_start++;
            }
            char *value_end = strstr(value_start, "\r\n");
            if (!value_end) {
                return -1;
            }

            size_t old_len = (size_t)(value_end - value_start);
            size_t new_len = strlen(host_value);
            size_t new_total = (size_t)s->request_len - old_len + new_len;
            if (new_total >= BUF_SIZE) {
                return -1;
            }

            memmove(value_start + new_len, value_end,
                    (size_t)s->request_len - (size_t)(value_end - s->request_buf) + 1);
            memcpy(value_start, host_value, new_len);
            s->request_len = (int)new_total;
            return 0;
        }

        line = strstr(line, "\r\n");
    }

    {
        const char *prefix = "\r\nHost: ";
        size_t prefix_len = strlen(prefix);
        size_t value_len = strlen(host_value);
        size_t insert_len = prefix_len + value_len;

        if ((size_t)s->request_len + insert_len >= BUF_SIZE) {
            return -1;
        }

        memmove(headers_end + insert_len, headers_end,
                (size_t)s->request_len - (size_t)(headers_end - s->request_buf) + 1);
        memcpy(headers_end, prefix, prefix_len);
        memcpy(headers_end + prefix_len, host_value, value_len);
        s->request_len += (int)insert_len;
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
        else if (s->state == STATE_SERVE_CACHE || s->state == STATE_FETCH_SERVER) {
            pfds[client_idx].events |= POLLOUT;
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

    if (s->state == STATE_FETCH_SERVER || s->state == STATE_SERVE_CACHE) {
        if (s->cache_entry && s->cache_entry->is_failed) {
            close_session(s);
            return;
        }   
    }

    if (server_idx >= 0 && (pfds[server_idx].revents & (POLLERR | POLLNVAL | POLLHUP))) {
        if (s->state == STATE_FETCH_SERVER && s->cache_entry) {
            s->cache_entry->is_failed = 1;
        }
        close_session(s);
        return;
    }

    if (s->state != STATE_READ_REQUEST) {
        if (client_idx >= 0 && (pfds[client_idx].revents & (POLLERR | POLLNVAL | POLLHUP))) {
            close_session(s);
            return;
        }
    }

    switch (s->state) {
        case STATE_READ_REQUEST:
            if (client_idx >= 0 && (pfds[client_idx].revents & POLLIN)) {
                if (s->request_len >= BUF_SIZE - 1) {
                    fprintf(stderr, "Request headers too large\n");
                    send_http_error(s, "400 Bad Request");
                    close_session(s);
                    break;
                }

                int bytes_read = recv(s->fd_client, s->request_buf + s->request_len, 
                                     BUF_SIZE - 1 - s->request_len, 0);

                if (bytes_read > 0) {
                    s->request_len += bytes_read;
                    s->request_buf[s->request_len] = '\0';

                    if (strstr(s->request_buf, "\r\n\r\n") == NULL) {
                        break;
                    }

                    int parse_code = parse_http_request(s);

                    if (parse_code == 1) {
                        char target_host[256];
                        char target_port[16];
                        const char *uri_to_parse = s->uri;

                        int parsed = parse_target_from_uri(uri_to_parse, target_host,
                                                          sizeof(target_host), target_port,
                                                          sizeof(target_port));

                        if (parsed != 1) {
                            fprintf(stderr, "Invalid absolute URI: %s\n", uri_to_parse);
                            send_http_error(s, "400 Bad Request");
                            close_session(s);
                            break;
                        }

                        const char *host_to_use = target_host;
                        const char *port_to_use = target_port;

                        char cache_key[MAX_URI_LEN + 300];
                        if (snprintf(cache_key, sizeof(cache_key), "%s", uri_to_parse) >= 
                            (int)sizeof(cache_key)) {
                            close_session(s);
                            break;
                        }

                        {
                            const char *path_start = strchr(uri_to_parse + strlen("http://"), '/');
                            if (!path_start) {
                                path_start = "/";
                            }
                            if (replace_request_target(s, path_start) < 0) {
                                close_session(s);
                                break;
                            }
                        }

                        if (set_host_header(s, host_to_use, port_to_use) < 0) {
                            close_session(s);
                            break;
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

                        s->fd_server = socket(target_addrinfo->ai_family, 
                                             target_addrinfo->ai_socktype, 
                                             target_addrinfo->ai_protocol);
                        if (s->fd_server < 0) {
                            freeaddrinfo(target_addrinfo);
                            close_session(s);
                            break;
                        }

                        set_nonblock(s->fd_server);

                        if (connect(s->fd_server, target_addrinfo->ai_addr, 
                                   target_addrinfo->ai_addrlen) < 0) {
                            if (errno != EINPROGRESS) {
                                freeaddrinfo(target_addrinfo);
                                close_session(s);
                                break;
                            }
                            s->connect_pending = 1;
                        } else {
                            s->connect_pending = 0;
                        }

                        freeaddrinfo(target_addrinfo);

                        s->request_sent = 0;
                        s->state = STATE_SEND_REQUEST;
                    } else if (parse_code == -1) {
                        send_http_error(s, "400 Bad Request");
                        close_session(s);
                    }
                } else if (bytes_read == 0) {
                    close_session(s);
                } else {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        close_session(s);
                    }
                }
            } else if (client_idx >= 0 && (pfds[client_idx].revents & (POLLERR | POLLNVAL | POLLHUP))) {
                close_session(s);
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
                        s->cache_entry->is_failed = 1;
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
                        s->cache_entry->is_failed = 1;
                        close_session(s);
                        break;
                    }
                }
            }

            if (client_idx >= 0) {
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
            if (s->cache_entry->is_failed) {
                close_session(s);
                break;
            }

            if (client_idx >= 0) {
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
    sessions[free_idx].version[0] = '\0';
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