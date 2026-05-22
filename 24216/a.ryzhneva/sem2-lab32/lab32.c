#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#define BUF_SIZE 8192
#define MAX_URI_LEN 2048
#define MAX_METHOD_LEN 16
#define MAX_VERSION_LEN 16

volatile sig_atomic_t server_running = 1;

typedef struct CacheEntry {
    char *uri;
    char *data;
    size_t size;
    size_t capacity;
    int is_complete;
    int is_failed;
    int reference_count;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    
    struct CacheEntry *next;
} CacheEntry;

CacheEntry *cache_head = NULL;
pthread_mutex_t cache_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static const char *default_target_host = NULL;
static const char *default_target_port = NULL;

void sig_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        server_running = 0;
    }
}

void close_connection(int fd) {
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

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

    pthread_mutex_init(&new_entry->mutex, NULL);
    pthread_cond_init(&new_entry->cond, NULL);

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
    pthread_mutex_lock(&entry->mutex);
    entry->reference_count--;
    pthread_mutex_unlock(&entry->mutex);
}

void cache_free_all() {
    pthread_mutex_lock(&cache_list_mutex);
    CacheEntry *curr = cache_head;
    while (curr != NULL) {
        CacheEntry *next = curr->next;
        free(curr->uri);
        free(curr->data);
        pthread_mutex_destroy(&curr->mutex);
        pthread_cond_destroy(&curr->cond);
        free(curr);
        curr = next;
    }
    cache_head = NULL;
    pthread_mutex_unlock(&cache_list_mutex);
    pthread_mutex_destroy(&cache_list_mutex);
}

int parse_request(int client_fd, char *req_buf, size_t req_buf_size, int *req_len,
                  char *method, size_t method_len, char *uri, size_t uri_len,
                  char *version, size_t version_len) {
    *req_len = 0;

    while (1) {
        int bytes_read = recv(client_fd, req_buf + *req_len, req_buf_size - 1 - *req_len, 0);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (bytes_read == 0) {
            return -1;
        }

        *req_len += bytes_read;
        req_buf[*req_len] = '\0';

        if (strstr(req_buf, "\r\n\r\n") != NULL) {
            break;
        }

        if (*req_len >= (int)req_buf_size - 1) {
            return -1;
        }
    }

    if (sscanf(req_buf, "%15s %2047s %15s", method, uri, version) != 3) {
        return -1;
    }

    if (strncmp(method, "GET", 3) != 0) {
        return -1;
    }

    (void)method_len;
    (void)uri_len;
    (void)version_len;
    return 0;
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
            if (!line_end) {
                return -1;
            }

            const char *colon = memchr(p, ':', (size_t)(line_end - p));
            if (colon) {
                size_t host_len_part = (size_t)(colon - p);
                size_t port_len_part = (size_t)(line_end - colon - 1);
                if (host_len_part == 0 || port_len_part == 0) {
                    return -1;
                }
                if (host_len_part >= host_len || port_len_part >= port_len) {
                    return -1;
                }

                memcpy(host, p, host_len_part);
                host[host_len_part] = '\0';
                memcpy(port, colon + 1, port_len_part);
                port[port_len_part] = '\0';
            } else {
                size_t host_len_part = (size_t)(line_end - p);
                if (host_len_part == 0 || host_len_part >= host_len) {
                    return -1;
                }

                memcpy(host, p, host_len_part);
                host[host_len_part] = '\0';
                if (snprintf(port, port_len, "%s", "80") >= (int)port_len) {
                    return -1;
                }
            }
            return 1;
        }
    }
    return 0;
}

int replace_request_target(char *req_buf, int *req_len, size_t buf_size, const char *new_target) {
    char *line_end = strstr(req_buf, "\r\n");
    if (!line_end) {
        return -1;
    }

    char *first_space = memchr(req_buf, ' ', (size_t)(line_end - req_buf));
    if (!first_space) {
        return -1;
    }

    char *second_space = memchr(first_space + 1, ' ', (size_t)(line_end - (first_space + 1)));
    if (!second_space) {
        return -1;
    }

    size_t old_len = (size_t)(second_space - (first_space + 1));
    size_t new_len = strlen(new_target);
    size_t new_total = (size_t)(*req_len) - old_len + new_len;

    if (new_total >= buf_size) {
        return -1;
    }

    memmove(first_space + 1 + new_len, second_space,
            (size_t)(*req_len) - (size_t)(second_space - req_buf) + 1);
    memcpy(first_space + 1, new_target, new_len);
    *req_len = (int)new_total;
    return 0;
}

int set_host_header(char *req_buf, int *req_len, size_t buf_size, const char *host, const char *port) {
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

    char *headers_end = strstr(req_buf, "\r\n\r\n");
    if (!headers_end) {
        return -1;
    }

    char *line = strstr(req_buf, "\r\n");
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
            size_t new_total = (size_t)(*req_len) - old_len + new_len;
            if (new_total >= buf_size) {
                return -1;
            }

            memmove(value_start + new_len, value_end,
                    (size_t)(*req_len) - (size_t)(value_end - req_buf) + 1);
            memcpy(value_start, host_value, new_len);
            *req_len = (int)new_total;
            return 0;
        }

        line = strstr(line, "\r\n");
    }

    {
        const char *prefix = "\r\nHost: ";
        size_t prefix_len = strlen(prefix);
        size_t value_len = strlen(host_value);
        size_t insert_len = prefix_len + value_len;

        if ((size_t)(*req_len) + insert_len >= buf_size) {
            return -1;
        }

        memmove(headers_end + insert_len, headers_end,
                (size_t)(*req_len) - (size_t)(headers_end - req_buf) + 1);
        memcpy(headers_end, prefix, prefix_len);
        memcpy(headers_end + prefix_len, host_value, value_len);
        *req_len += (int)insert_len;
    }

    return 0;
}

int set_connection_close(char *req_buf, int *req_len, size_t buf_size) {
    if (strstr(req_buf, "\r\nConnection:") || strstr(req_buf, "\r\nconnection:")) {
        return 0;
    }

    char *headers_end = strstr(req_buf, "\r\n\r\n");
    if (!headers_end) {
        return -1;
    }

    const char *line = "\r\nConnection: close";
    size_t line_len = strlen(line);
    if ((size_t)(*req_len) + line_len >= buf_size) {
        return -1;
    }

    memmove(headers_end + line_len, headers_end,
            (size_t)(*req_len) - (size_t)(headers_end - req_buf) + 1);
    memcpy(headers_end, line, line_len);
    *req_len += (int)line_len;
    return 0;
}

void mark_cache_failed(CacheEntry *entry) {
    if (!entry) {
        return;
    }
    pthread_mutex_lock(&entry->mutex);
    entry->is_failed = 1;
    entry->is_complete = 1;
    pthread_cond_broadcast(&entry->cond);
    pthread_mutex_unlock(&entry->mutex);
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
    
    return listen_fd;
}

ssize_t send_all(int sockfd, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = send(sockfd, buf + total, len - total, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) {
                continue;
            }
            return -1;
        }
        total += n;
    }
    return total;
}

void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    
    char req_buf[BUF_SIZE];
    int  req_len = 0;
    char method[MAX_METHOD_LEN];
    char uri[MAX_URI_LEN];
    char version[MAX_VERSION_LEN];
    

    if (parse_request(client_fd, req_buf, sizeof(req_buf), &req_len,
                      method, sizeof(method), uri, sizeof(uri),
                      version, sizeof(version)) != 0) {
        close_connection(client_fd);
        free(arg);
        return NULL;
    }

    char target_host[256];
    char target_port[16];
    int parsed = parse_target_from_uri(uri, target_host, sizeof(target_host), target_port, sizeof(target_port));
    int uri_is_absolute = (parsed == 1);
    if (parsed == -1) {
        close_connection(client_fd);
        free(arg);
        return NULL;
    } else if (parsed == 0) {
        parsed = parse_host_header(req_buf, target_host, sizeof(target_host), target_port, sizeof(target_port));
        if (parsed == -1) {
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
    }

    const char *host_to_use = (parsed == 1) ? target_host : default_target_host;
    const char *port_to_use = (parsed == 1) ? target_port : default_target_port;
    if (!host_to_use || !port_to_use) {
        close_connection(client_fd);
        free(arg);
        return NULL;
    }

    if (uri_is_absolute) {
        const char *path_start = strchr(uri + strlen("http://"), '/');
        if (!path_start) {
            path_start = "/";
        }
        if (replace_request_target(req_buf, &req_len, sizeof(req_buf), path_start) < 0) {
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
    }

    if (set_host_header(req_buf, &req_len, sizeof(req_buf), host_to_use, port_to_use) < 0) {
        close_connection(client_fd);
        free(arg);
        return NULL;
    }

    if (set_connection_close(req_buf, &req_len, sizeof(req_buf)) < 0) {
        close_connection(client_fd);
        free(arg);
        return NULL;
    }

    char cache_key[MAX_URI_LEN + 300];
    if (uri_is_absolute) {
        if (snprintf(cache_key, sizeof(cache_key), "%s", uri) >= (int)sizeof(cache_key)) {
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
    } else {
        const char *path_prefix = (uri[0] == '/') ? "" : "/";
        if (snprintf(cache_key, sizeof(cache_key), "http://%s:%s%s%s", host_to_use, port_to_use, path_prefix, uri) >= (int)sizeof(cache_key)) {
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
    }

    pthread_mutex_lock(&cache_list_mutex);
    CacheEntry *entry = cache_lookup(cache_key);
    int is_writer = 0;

    if (entry != NULL) {
        pthread_mutex_lock(&entry->mutex);
        entry->reference_count++;
        pthread_mutex_unlock(&entry->mutex);
    } else {
        entry = cache_add_entry(cache_key);
        if (!entry) {
            pthread_mutex_unlock(&cache_list_mutex);
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
        is_writer = 1;
    }
    pthread_mutex_unlock(&cache_list_mutex);

    if (is_writer) {
        struct addrinfo *target_addrinfo = resolve_target(host_to_use, port_to_use);
        int server_fd = -1;
        if (target_addrinfo) {
            server_fd = socket(target_addrinfo->ai_family, target_addrinfo->ai_socktype, target_addrinfo->ai_protocol);
        }

        if (!target_addrinfo || server_fd < 0) {
            mark_cache_failed(entry);
            const char *err_502 = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
            send_all(client_fd, err_502, strlen(err_502));
        } else if (connect(server_fd, target_addrinfo->ai_addr, target_addrinfo->ai_addrlen) == 0) {

            if (send_all(server_fd, req_buf, req_len) < 0) {
                close_connection(server_fd);
                server_fd = -1;
                mark_cache_failed(entry);
                const char *err_502 = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
                send_all(client_fd, err_502, strlen(err_502));
            }

            char temp_buf[BUF_SIZE];
            int client_active = 1;
            
            while (server_fd >= 0) {
                int bytes_recv = recv(server_fd, temp_buf, sizeof(temp_buf), 0);
                
                if (bytes_recv < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    mark_cache_failed(entry);
                    break;
                } else if (bytes_recv == 0) {
                    break;
                }
                
                pthread_mutex_lock(&entry->mutex);
                
                int oom_error = (cache_append_data(entry, temp_buf, bytes_recv) != 0);
                if (!oom_error) {
                    pthread_cond_broadcast(&entry->cond);
                }
                
                pthread_mutex_unlock(&entry->mutex);
                
                if (oom_error) {
                    fprintf(stderr, "Ошибка: не хватает памяти для кэширования ресурса.\n");
                    mark_cache_failed(entry);
                    break;
                }

                if (client_active) {
                    if (send_all(client_fd, temp_buf, bytes_recv) < 0) {
                        client_active = 0;
                    }
                }
            }
            if (server_fd >= 0) {
                close_connection(server_fd);
            }

        } else {
            if (server_fd >= 0) {
                close_connection(server_fd);
            }
            mark_cache_failed(entry);
            const char *err_502 = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
            send_all(client_fd, err_502, strlen(err_502));
        }

        if (target_addrinfo) {
            freeaddrinfo(target_addrinfo);
        }

        pthread_mutex_lock(&entry->mutex);
        entry->is_complete = 1;
        pthread_cond_broadcast(&entry->cond);
        pthread_mutex_unlock(&entry->mutex);

    } else {
        size_t offset = 0;
        char local_buf[BUF_SIZE];

        while (1) {
            pthread_mutex_lock(&entry->mutex);
            
            while (offset == entry->size && !entry->is_complete) {
                pthread_cond_wait(&entry->cond, &entry->mutex);
            }

            if (entry->is_failed) {
                pthread_mutex_unlock(&entry->mutex);
                const char *err_502 = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
                send_all(client_fd, err_502, strlen(err_502));
                break;
            }
            
            if (offset == entry->size && entry->is_complete) {
                pthread_mutex_unlock(&entry->mutex);
                break;
            }
            
            size_t chunk = entry->size - offset;
            if (chunk > BUF_SIZE) {
                chunk = BUF_SIZE;
            }
            
            memcpy(local_buf, entry->data + offset, chunk);
            pthread_mutex_unlock(&entry->mutex);
            
            if (send_all(client_fd, local_buf, chunk) < 0) {
                break; 
            }
            offset += chunk;
        }
    }

    cache_release_entry(entry);
    
    close_connection(client_fd);
    free(arg);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <listen_port> <target_host> <target_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    default_target_host = argv[2];
    default_target_port = argv[3];
    int listen_fd = init_listen_socket(argv[1]);

    while(server_running) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Ошибка accept");
            break;
        }

        int *arg = malloc(sizeof(int));
        if (!arg) {
            close_connection(client_fd);
            continue;
        }
        *arg = client_fd;

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&tid, &attr, handle_client, arg) != 0) {
            perror("Ошибка pthread_create. Лимит потоков исчерпан.");
            const char *err_msg = "HTTP/1.0 503 Service Unavailable\r\n\r\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
            close_connection(client_fd);
            free(arg);
        }
        pthread_attr_destroy(&attr);
    }

    close_connection(listen_fd);
    cache_free_all();

    return 0;
}