#include "cache.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFFER_SIZE 8192

Cache *cache;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

const char* get_header_value(const char *headers, const char *header_name) {
    char search[64];
    snprintf(search, sizeof(search), "\r\n%s:", header_name);
    
    const char *pos = strcasestr(headers, search);
    if (!pos) {
        snprintf(search, sizeof(search), "%s:", header_name);
        pos = strcasestr(headers, search);
        if (!pos) return NULL;
    }
    
    pos += strlen(search);
    while (*pos == ' ' || *pos == '\t') pos++;
    
    const char *end = pos;
    while (*end && *end != '\r' && *end != '\n') end++;
    
    static char value[256];
    size_t len = end - pos;
    if (len >= sizeof(value)) len = sizeof(value) - 1;
    strncpy(value, pos, len);
    value[len] = '\0';
    return value;
}

ssize_t parse_response_headers(const char *buf, size_t len, size_t *content_length) {
    *content_length = 0;
    
    const char *headers_end = strstr(buf, "\r\n\r\n");
    if (!headers_end) {
        headers_end = strstr(buf, "\n\n");
        if (!headers_end) return -1;
    }
    
    size_t headers_len = headers_end - buf + 4;
    if (headers_len > len) return -1;
    
    const char *cl = get_header_value(buf, "Content-Length");
    if (cl) {
        *content_length = strtoull(cl, NULL, 10);
    }
    
    return headers_len;
}

int client_wants_keepalive(const char *req_buf) {
    const char *conn = get_header_value(req_buf, "Connection");
    const char *proxy_conn = get_header_value(req_buf, "Proxy-Connection");
    
    if (proxy_conn) {
        return strcasecmp(proxy_conn, "keep-alive") == 0;
    }
    if (conn) {
        return strcasecmp(conn, "keep-alive") == 0;
    }
    
    return (strstr(req_buf, "HTTP/1.1") != NULL);
}

int parse_http_request(const char *request, char *host, int host_len, char *path, int path_len) {
    char method[16], url[256];
    
    if (sscanf(request, "%15s %255s", method, url) != 2) {
        return -1;
    }
    
    if (strcmp(method, "GET") != 0) {
        return -2;
    }
    
    if (strncmp(url, "http://", 7) != 0) {
        return -4;
    }

    char *host_start = url + 7;
    char *path_start = strchr(host_start, '/');
    
    if (path_start) {
        int host_length = path_start - host_start;
        if (host_length < host_len) {
            strncpy(host, host_start, host_length);
            host[host_length] = '\0';
            strncpy(path, path_start, path_len);
            path[path_len - 1] = '\0';
        } else {
            return -3;
        }
    } else {
        strncpy(host, host_start, host_len);
        host[host_len - 1] = '\0';
        strncpy(path, "/", path_len);
    }
    
    return 0;
}

int connect_to_server(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct hostent *server = gethostbyname(host);
    if (!server) {
        close(sock);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

void send_error_response(int client_fd, int code, const char *message) {
    char response[512];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "<html><body><h1>%d %s</h1></body></html>",
        code, message, strlen(message) + 50, code, message);
    
    send(client_fd, response, len, 0);
}

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} ClientThreadArgs;

void* handle_client(void *arg) {
    ClientThreadArgs *args = (ClientThreadArgs*)arg;
    int client_fd = args->client_fd;
    free(args);
    
    printf("[Thread %d] Client connected, fd=%d\n", pthread_self(), client_fd);
    
    char req_buf[BUFFER_SIZE];
    char *resp_buf = NULL;
    size_t resp_cap = 0;
    size_t resp_len = 0;

    while (1) {
        memset(req_buf, 0, sizeof(req_buf));
        size_t req_len = 0;
        resp_len = 0;
        
        while (1) {
            ssize_t n = recv(client_fd, req_buf + req_len, BUFFER_SIZE - req_len - 1, 0);
            if (n <= 0) {
                if (n == 0) {
                    printf("[Thread %d] Client fd=%d closed connection\n", pthread_self(), client_fd);
                } else {
                    perror("[Thread] recv");
                }
                goto cleanup;
            }
            
            req_len += n;
            req_buf[req_len] = '\0';
            
            if (strstr(req_buf, "\r\n\r\n") || strstr(req_buf, "\n\n")) {
                break;
            }
        }
        
        printf("[Thread %d] Received request (%zu bytes)\n", pthread_self(), req_len);

        char host[128] = {0};
        char path[256] = {0};
        
        if (parse_http_request(req_buf, host, sizeof(host), path, sizeof(path)) != 0) {
            send_error_response(client_fd, 400, "Bad Request");
            break;
        }
        
        char url[512];
        snprintf(url, sizeof(url), "http://%s%s", host, path);
        printf("[Thread %d] Request: %s\n", pthread_self(), url);

        char *cached_data = NULL;
        size_t cached_len = 0;
        
        pthread_mutex_lock(&cache_mutex);
        CacheEntry *cached = get_entry(cache, url);
        if (cached) {
            cached_data = cached->data;
            cached_len = cached->data_size;
            printf("[Thread %d] Cache HIT: %s\n", pthread_self(), url);
        }
        pthread_mutex_unlock(&cache_mutex);
        
        if (cached_data) {
            if (send(client_fd, cached_data, cached_len, 0) < 0) {
                perror("[Thread] send cached");
                break;
            }

            if (!client_wants_keepalive(req_buf)) {
                break;
            }
            continue;
        }
        
        printf("[Thread %d] Cache MISS: %s\n", pthread_self(), url);

        int server_fd = connect_to_server(host, 80);
        if (server_fd < 0) {
            send_error_response(client_fd, 502, "Bad Gateway");
            break;
        }
        
        printf("[Thread %d] Connected to upstream server\n", pthread_self());
        
        if (send(server_fd, req_buf, req_len, 0) < 0) {
            perror("[Thread] send to server");
            close(server_fd);
            break;
        }
        
        size_t expected_body_len = 0;
        size_t headers_end_pos = 0;
        int server_wants_close = 0;
        
        while (1) {
            if (resp_len + BUFFER_SIZE > resp_cap) {
                resp_cap = resp_cap ? resp_cap * 2 : BUFFER_SIZE * 2;
                char *new_buf = realloc(resp_buf, resp_cap);
                if (!new_buf) {
                    perror("[Thread] realloc");
                    close(server_fd);
                    break;
                }
                resp_buf = new_buf;
            }
            
            ssize_t n = recv(server_fd, resp_buf + resp_len, BUFFER_SIZE, 0);
            if (n < 0) {
                perror("[Thread] recv from server");
                close(server_fd);
                break;
            }
            
            if (n == 0) {
                server_wants_close = 1;
                printf("[Thread %d] Server closed connection\n", pthread_self());
                break;
            }
            
            resp_len += n;

            if (headers_end_pos == 0) {
                ssize_t headers_end = parse_response_headers(resp_buf, resp_len, &expected_body_len);
                if (headers_end > 0) {
                    headers_end_pos = headers_end;
                    
                    const char *conn = get_header_value(resp_buf, "Connection");
                    if (conn && strcasecmp(conn, "close") == 0) {
                        server_wants_close = 1;
                    }
                }
            }
            
            if (headers_end_pos > 0 && expected_body_len > 0) {
                size_t body_received = resp_len - headers_end_pos;
                if (body_received >= expected_body_len) {
                    printf("[Thread %d] Response complete (%zu bytes)\n", pthread_self(), resp_len);
                    break;
                }
            }
        }
        
        close(server_fd);

        if (resp_len > 0 && !server_wants_close) {
            pthread_mutex_lock(&cache_mutex);
            char *url_copy = strdup(url);
            char *data_copy = malloc(resp_len);
            if (url_copy && data_copy) {
                memcpy(data_copy, resp_buf, resp_len);
                CacheEntry *entry = create_entry(data_copy, resp_len, url_copy);
                CacheEntry *removed = add_entry(cache, entry);
                if (removed) {
                    free_entry(removed);
                }
                printf("[Thread %d] Cached response (%zu bytes)\n", pthread_self(), resp_len);
            } else {
                free(url_copy);
                free(data_copy);
            }
            pthread_mutex_unlock(&cache_mutex);
        }

        if (resp_len > 0) {
            if (send(client_fd, resp_buf, resp_len, 0) < 0) {
                perror("[Thread] send to client");
                break;
            }
            printf("[Thread %d] Sent response to client\n", pthread_self());
        }

        if (!client_wants_keepalive(req_buf) || server_wants_close) {
            printf("[Thread %d] Closing connection (keep-alive=%d, server_close=%d)\n", 
                   pthread_self(), client_wants_keepalive(req_buf), server_wants_close);
            break;
        }
        
        printf("[Thread %d] Keeping connection alive for next request\n", pthread_self());
    }
    
cleanup:
    close(client_fd);
    if (resp_buf) free(resp_buf);
    printf("[Thread %d] Client fd=%d disconnected\n", pthread_self(), client_fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    cache = init_cache(10);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    
    if (listen(listen_fd, 0) < 0) {
        perror("listen");
        return 1;
    }
    
    printf("Multi-threaded proxy server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        printf("[Main] New connection from %s:%d, fd=%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);
        
        ClientThreadArgs *args = malloc(sizeof(ClientThreadArgs));
        if (!args) {
            perror("malloc");
            close(client_fd);
            continue;
        }
        args->client_fd = client_fd;
        args->client_addr = client_addr;
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        pthread_t thread;
        if (pthread_create(&thread, &attr, handle_client, args) != 0) {
            perror("pthread_create");
            free(args);
            close(client_fd);
            continue;
        }
        
        pthread_attr_destroy(&attr);
    }
    
    close(listen_fd);
    free_cache(cache);
    pthread_mutex_destroy(&cache_mutex);
    return 0;
}
