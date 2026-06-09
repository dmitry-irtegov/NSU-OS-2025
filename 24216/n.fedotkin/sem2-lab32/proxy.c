#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "cache.h"

#define DEFAULT_PORT   8080
#define MAX_WORKERS    100
#define BUF_SIZE       65536
#define GROW_SIZE      65536

volatile sig_atomic_t g_running = 1;

void sig_handler(int signum) {
    (void)signum;
    g_running = 0;
}

cache_t g_cache;

sem_t g_worker_slots;

typedef struct {
    int client_fd;
} worker_args_t;

ssize_t send_all(int fd, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) {
            return n;
        }
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

void close_socket(int fd) {
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

int parse_target_from_uri(const char* uri, char* host, size_t host_len, int* port) {
    const char* scheme = "http://";
    size_t scheme_len = strlen(scheme);

    if (strncmp(uri, scheme, scheme_len) != 0) {
        return -1;
    }

    const char* host_start = uri + scheme_len;
    const char* path_start = strchr(host_start, '/');
    const char* host_end = path_start ? path_start : uri + strlen(uri);
    const char* colon = memchr(host_start, ':', (size_t)(host_end - host_start));

    if (host_start == host_end) return -1;

    if (colon) {
        size_t host_part_len = (size_t)(colon - host_start);
        size_t port_part_len = (size_t)(host_end - colon - 1);

        if (host_part_len == 0 || port_part_len == 0 || host_part_len >= host_len) return -1;

        memcpy(host, host_start, host_part_len);
        host[host_part_len] = '\0';

        char port_str[16] = {0};
        if (port_part_len >= sizeof(port_str)) return -1;
        memcpy(port_str, colon + 1, port_part_len);
        *port = atoi(port_str);
    } else {
        size_t host_part_len = (size_t)(host_end - host_start);
        if (host_part_len == 0 || host_part_len >= host_len) return -1;

        memcpy(host, host_start, host_part_len);
        host[host_part_len] = '\0';
        *port = 80;
    }
    return 1;
}

int parse_request(const char* request, char* host, size_t host_len, int* port, char* cache_key, size_t key_len) {
    char method[16], uri[4096], version[16];

    if (sscanf(request, "%15s %4095s %15s", method, uri, version) != 3) {
        return -1;
    }

    if (strncmp(method, "GET", 3) != 0) {
        fprintf(stderr, "Unsupported HTTP method: %s\n", method);
        return -1;
    }

    if (strcmp(version, "HTTP/1.0") != 0) {
        fprintf(stderr, "Unsupported HTTP version: %s\n", version);
        return -1;
    }

    if (parse_target_from_uri(uri, host, host_len, port) != 1) {
        fprintf(stderr, "Error: request URI must be absolute (http://host[:port]/path)\n");
        return -1;
    }

    snprintf(cache_key, key_len, "%s", uri);
    return 0;
}

int connect_to_server(const char* host, int port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%d", port);

    int err = getaddrinfo(host, portstr, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Error getaddrinfo '%s': %s\n", host, gai_strerror(err));
        return -1;
    }

    int sfd = -1;
    for (struct addrinfo* rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue;
        }
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(sfd);
        sfd = -1;
    }
    freeaddrinfo(res);

    if (sfd == -1) {
        fprintf(stderr, "Error connect to '%s': all addresses failed\n", host);
    }
    return sfd;
}

int fetch_complete_response(int server_fd, char** out_data, size_t* out_len) {
    char* response = NULL;
    size_t len = 0;
    size_t cap = 0;
    char tmp[BUF_SIZE];

    while (1) {
        ssize_t n = recv(server_fd, tmp, sizeof(tmp), 0);
        if (n < 0) {
            perror("Error recv from server");
            free(response);
            return -1;
        }
        if (n == 0) {
            break;
        }

        if (len + (size_t)n > cap) {
            size_t newcap = cap + GROW_SIZE;
            while (newcap < len + (size_t)n) {
                newcap += GROW_SIZE;
            }
            char* newbuf = realloc(response, newcap);
            if (newbuf == NULL) {
                perror("Error realloc response");
                free(response);
                return -1;
            }
            response = newbuf;
            cap = newcap;
        }

        memcpy(response + len, tmp, (size_t)n);
        len += (size_t)n;
    }

    *out_data = response;
    *out_len = len;
    return 0;
}

int read_request(int client_fd, char* request, size_t cap, int* out_len) {
    int len = 0;

    while (1) {
        ssize_t n = recv(client_fd, request + len, cap - (size_t)len - 1, 0);
        if (n < 0) {
            perror("Error recv from client");
            return -1;
        }
        if (n == 0) {
            return -1;
        }

        len += (int)n;
        request[len] = '\0';

        if (strstr(request, "\r\n\r\n") != NULL) {
            break;
        }
        if ((size_t)len >= cap - 1) {
            fprintf(stderr, "Error: request too large\n");
            return -1;
        }
    }

    *out_len = len;
    return 0;
}

void* worker_routine(void* arg) {
    worker_args_t* params = (worker_args_t*)arg;
    int client_fd = params->client_fd;
    free(params);

    char request[BUF_SIZE];
    int request_len = 0;
    char host[256];
    int port = 80;
    char cache_key[4352];

    if (read_request(client_fd, request, sizeof(request), &request_len) == -1) {
        close_socket(client_fd);
        sem_post(&g_worker_slots);
        return NULL;
    }

    if (parse_request(request, host, sizeof(host), &port, cache_key, sizeof(cache_key)) == -1) {
        fprintf(stderr, "Error: cannot parse request\n");
        close_socket(client_fd);
        sem_post(&g_worker_slots);
        return NULL;
    }

    char* response = NULL;
    size_t response_len = 0;

    int hit = cache_get_copy(&g_cache, cache_key, &response, &response_len);
    if (hit == 1) {
        if (send_all(client_fd, response, response_len) < 0) {
            perror("Error send cached response to client");
        }
        free(response);
        close_socket(client_fd);
        sem_post(&g_worker_slots);
        return NULL;
    }

    int server_fd = connect_to_server(host, port);
    if (server_fd == -1) {
        close_socket(client_fd);
        sem_post(&g_worker_slots);
        return NULL;
    }

    if (send_all(server_fd, request, (size_t)request_len) < 0) {
        perror("Error send request to server");
        close_socket(server_fd);
        close_socket(client_fd);
        sem_post(&g_worker_slots);
        return NULL;
    }

    if (fetch_complete_response(server_fd, &response, &response_len) == -1) {
        close_socket(server_fd);
        close_socket(client_fd);
        sem_post(&g_worker_slots);
        return NULL;
    }
    close_socket(server_fd);

    if (response_len > 0) {
        if (send_all(client_fd, response, response_len) < 0) {
            perror("Error send response to client");
        } else {
            cache_put(&g_cache, cache_key, response, response_len);
        }
    }

    free(response);
    close_socket(client_fd);
    sem_post(&g_worker_slots);
    return NULL;
}

int create_listen_socket(int port) {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("Error socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error setsockopt SO_REUSEADDR");
        close(lfd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Error bind");
        close(lfd);
        return -1;
    }

    if (listen(lfd, 64) == -1) {
        perror("Error listen");
        close(lfd);
        return -1;
    }

    return lfd;
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Error: invalid port %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (cache_init(&g_cache) == -1) {
        return EXIT_FAILURE;
    }

    if (sem_init(&g_worker_slots, 0, MAX_WORKERS) != 0) {
        perror("Error init worker slots sem");
        cache_destroy(&g_cache);
        return EXIT_FAILURE;
    }

    int lfd = create_listen_socket(port);
    if (lfd == -1) {
        sem_destroy(&g_worker_slots);
        cache_destroy(&g_cache);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Proxy listening on port %d\n", port);

    while (g_running) {
        if (sem_wait(&g_worker_slots) != 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error sem_wait worker slots");
            break;
        }

        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cfd = accept(lfd, (struct sockaddr*)&caddr, &clen);
        if (cfd == -1) {
            sem_post(&g_worker_slots);
            if (errno == EINTR) {
                continue;
            }
            perror("Error accept");
            continue;
        }

        worker_args_t* params = malloc(sizeof(worker_args_t));
        if (params == NULL) {
            perror("Error malloc worker args");
            close_socket(cfd);
            sem_post(&g_worker_slots);
            continue;
        }
        params->client_fd = cfd;

        pthread_t tid;
        int res = pthread_create(&tid, NULL, worker_routine, params);
        if (res != 0) {
            fprintf(stderr, "Error pthread_create: %s\n", strerror(res));
            free(params);
            close_socket(cfd);
            sem_post(&g_worker_slots);
            continue;
        }

        res = pthread_detach(tid);
        if (res != 0) {
            fprintf(stderr, "Error pthread_detach: %s\n", strerror(res));
        }
    }

    fprintf(stderr, "Proxy shutting down\n");

    close_socket(lfd);

    for (int i = 0; i < MAX_WORKERS; i++) {
        if (sem_wait(&g_worker_slots) != 0 && errno != EINTR) {
            perror("Error sem_wait on shutdown");
            break;
        }
    }

    sem_destroy(&g_worker_slots);
    cache_destroy(&g_cache);
    return EXIT_SUCCESS;
}
