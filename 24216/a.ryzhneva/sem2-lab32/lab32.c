#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int reference_count;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    
    struct CacheEntry *next;
} CacheEntry;

CacheEntry *cache_head = NULL;
pthread_mutex_t cache_list_mutex = PTHREAD_MUTEX_INITIALIZER;
struct addrinfo *target_addrinfo = NULL;

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

void resolve_target(const char *target_host, const char *target_port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(target_host, target_port, &hints, &target_addrinfo) != 0) {
        perror("Ошибка getaddrinfo для целевого узла");
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
    int req_len = 0;
    char method[MAX_METHOD_LEN];
    char uri[MAX_URI_LEN];
    char version[MAX_VERSION_LEN];
    

    while (1) {
        int bytes_read = recv(client_fd, req_buf + req_len, BUF_SIZE - 1 - req_len, 0);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            close_connection(client_fd);
            free(arg);
            return NULL;
        } else if (bytes_read == 0) {
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
        
        req_len += bytes_read;
        req_buf[req_len] = '\0';
        
        if (strstr(req_buf, "\r\n\r\n") != NULL) {
            break;
        }
        
        if (req_len >= BUF_SIZE - 1) {
            close_connection(client_fd);
            free(arg);
            return NULL;
        }
    }

    if (sscanf(req_buf, "%15s %2047s %15s", method, uri, version) != 3 || strncmp(method, "GET", 3) != 0) {
        close_connection(client_fd);
        free(arg);
        return NULL;
    }

    pthread_mutex_lock(&cache_list_mutex);
    CacheEntry *entry = cache_lookup(uri);
    int is_writer = 0;

    if (entry != NULL) {
        pthread_mutex_lock(&entry->mutex);
        entry->reference_count++;
        pthread_mutex_unlock(&entry->mutex);
    } else {
        entry = (CacheEntry *)malloc(sizeof(CacheEntry));
        if (!entry) {
            pthread_mutex_unlock(&cache_list_mutex);
            close_connection(client_fd);
            free(arg);
            return NULL;
        }

        entry->uri = strdup(uri);
        entry->capacity = BUF_SIZE;
        entry->size = 0;
        entry->data = (char *)malloc(entry->capacity);

        if (!entry->uri || !entry->data) {
            if (entry->uri) {
                free(entry->uri);
            }
            if (entry->data) {
                free(entry->data);
            }
            free(entry);
            pthread_mutex_unlock(&cache_list_mutex);
            close_connection(client_fd);
            free(arg);
            return NULL;
        }

        entry->is_complete = 0;
        entry->reference_count = 1;
        
        pthread_mutex_init(&entry->mutex, NULL);
        pthread_cond_init(&entry->cond, NULL);
        
        entry->next = cache_head;
        cache_head = entry;
        is_writer = 1;
    }
    pthread_mutex_unlock(&cache_list_mutex);

    if (is_writer) {
        int server_fd = socket(target_addrinfo->ai_family, target_addrinfo->ai_socktype, target_addrinfo->ai_protocol);
        
        if (server_fd >= 0 && connect(server_fd, target_addrinfo->ai_addr, target_addrinfo->ai_addrlen) == 0) {

            if (send_all(server_fd, req_buf, req_len) < 0) {
                close_connection(server_fd);
                server_fd = -1;
            }

            char temp_buf[BUF_SIZE];
            int client_active = 1;
            
            while (server_fd >= 0) {
                int bytes_recv = recv(server_fd, temp_buf, sizeof(temp_buf), 0);
                
                if (bytes_recv < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                } else if (bytes_recv == 0) {
                    break;
                }
                
                pthread_mutex_lock(&entry->mutex);
                
                int oom_error = 0;
                if (entry->size + bytes_recv > entry->capacity) {
                    size_t new_cap = entry->capacity * 2;
                    while (entry->size + bytes_recv > new_cap) {
                        new_cap *= 2;
                    }

                    char *new_data = (char *)realloc(entry->data, new_cap);
                    if (new_data) {
                        entry->data = new_data;
                        entry->capacity = new_cap;
                    } else {
                        oom_error = 1;
                    }
                }
                
                if (!oom_error) {
                    memcpy(entry->data + entry->size, temp_buf, bytes_recv);
                    entry->size += bytes_recv;
                    pthread_cond_broadcast(&entry->cond);
                }
                
                pthread_mutex_unlock(&entry->mutex);
                
                if (oom_error) {
                    fprintf(stderr, "Ошибка: не хватает памяти для кэширования ресурса.\n");
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
            const char *err_502 = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
            send_all(client_fd, err_502, strlen(err_502));
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

    pthread_mutex_lock(&entry->mutex);
    entry->reference_count--;
    pthread_mutex_unlock(&entry->mutex);
    
    close_connection(client_fd);
    free(arg);
    return NULL;
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

    resolve_target(argv[2], argv[3]);
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

    if (target_addrinfo) {
        freeaddrinfo(target_addrinfo);
    }
    close_connection(listen_fd);
    cache_free_all();

    return 0;
}