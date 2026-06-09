#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "cache.h"

volatile sig_atomic_t g_running = 1;

void sig_handler(int signum) {
    (void)signum;
    g_running = 0;
}

#define DEFAULT_PORT       8080
#define MAX_CONN_PER_WORKER 256
#define BUF_SIZE           65536
#define GROW_SIZE          65536

typedef enum {
    STATE_CLIENT_READING,
    STATE_CONNECTING,
    STATE_SERVER_WRITING,
    STATE_SERVER_READING,
    STATE_CLIENT_WRITING,
    STATE_DONE
} conn_state_t;

typedef struct {
    int client_fd;
    int server_fd;
    conn_state_t state;
    char request[BUF_SIZE];
    int request_len;
    char *response;
    size_t response_len;
    size_t response_cap;
    size_t bytes_sent;
    char host[256];
    int port;
    char path[4096];
    char cache_key[4352];
    int from_cache;
} connection_t;

typedef struct {
    pthread_t tid;
    int id;
    int pipe_fd[2];
    connection_t conns[MAX_CONN_PER_WORKER];
    int nconns;
} worker_t;

cache_t g_cache;

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("Error fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("Error fcntl F_SETFL");
        return -1;
    }
    return 0;
}

int set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("Error fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        perror("Error fcntl F_SETFL");
        return -1;
    }
    return 0;
}

void close_connection(worker_t* w, int idx) {
    connection_t *c = &w->conns[idx];

    if (c->client_fd >= 0) {
        shutdown(c->client_fd, SHUT_RDWR);
        close(c->client_fd);
        c->client_fd = -1;
    }
    if (c->server_fd >= 0) {
        shutdown(c->server_fd, SHUT_RDWR);
        close(c->server_fd);
        c->server_fd = -1;
    }
    if (c->response != NULL) {
        free(c->response);
    }
    c->response = NULL;

    w->conns[idx] = w->conns[w->nconns - 1];
    w->nconns--;
}

int parse_target_from_uri(const char* uri, char* host, size_t host_len, int* port) {
    const char *scheme = "http://";
    size_t scheme_len = strlen(scheme);

    if (strncmp(uri, scheme, scheme_len) != 0) {
        return -1;
    }

    const char *host_start = uri + scheme_len;
    const char *path_start = strchr(host_start, '/');
    const char *host_end = path_start ? path_start : uri + strlen(uri);
    const char *colon = memchr(host_start, ':', (size_t)(host_end - host_start));

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

int parse_request(connection_t *c) {
    char method[16], uri[4096], version[16];

    if (sscanf(c->request, "%15s %4095s %15s", method, uri, version) != 3) {
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

    if (parse_target_from_uri(uri, c->host, sizeof(c->host), &c->port) != 1) {
        fprintf(stderr, "Error: request URI must be absolute (http://host[:port]/path)\n");
        return -1;
    }

    snprintf(c->cache_key, sizeof(c->cache_key), "%s", uri);

    strncpy(c->path, uri, sizeof(c->path) - 1);
    c->path[sizeof(c->path) - 1] = '\0';

    return 0;
}

int start_connect(connection_t* c) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%d", c->port);

    int err = getaddrinfo(c->host, portstr, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "Error getaddrinfo '%s': %s\n", c->host, gai_strerror(err));
        return -1;
    }

    int sfd = -1;
    for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            break;

        if (set_nonblocking(sfd) == -1) {
            close(sfd);
            sfd = -1;
            continue;
        }

        int ret = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (ret == 0 || errno == EINPROGRESS)
            break;

        close(sfd);
        sfd = -1;
    }
    freeaddrinfo(res);

    if (sfd == -1) {
        fprintf(stderr, "Error connect to '%s': all addresses failed\n", c->host);
        return -1;
    }

    c->server_fd = sfd;
    c->state = STATE_CONNECTING;
    return 0;
}

int append_response(connection_t* c, const char* buf, ssize_t n) {
    if (c->response_len + (size_t)n > c->response_cap) {
        size_t newcap = c->response_cap + GROW_SIZE;
        while (newcap < c->response_len + (size_t)n) {
            newcap += GROW_SIZE;
        }
        char *newbuf = realloc(c->response, newcap);
        if (newbuf == NULL) {
            perror("Error realloc response");
            return -1;
        }
        c->response = newbuf;
        c->response_cap = newcap;
    }
    memcpy(c->response + c->response_len, buf, n);
    c->response_len += n;
    return 0;
}

void accept_connection(worker_t* w, int cfd) {
    if (w->nconns >= MAX_CONN_PER_WORKER) {
        fprintf(stderr, "Worker %d: too many connections, dropping\n", w->id);
        shutdown(cfd, SHUT_RDWR);
        close(cfd);
        return;
    }

    connection_t *c = &w->conns[w->nconns];
    memset(c, 0, sizeof(*c));
    c->client_fd = cfd;
    c->server_fd = -1;
    c->state = STATE_CLIENT_READING;
    c->response = NULL;
    w->nconns++;
}

void handle_connection(worker_t* w, int i, fd_set* rset, fd_set* wset, char* tmp_buf) {
    connection_t *c = &w->conns[i];

    if (c->state == STATE_CLIENT_READING && FD_ISSET(c->client_fd, rset)) {
        ssize_t n = recv(c->client_fd, c->request + c->request_len,
                         BUF_SIZE - c->request_len - 1, 0);
        if (n <= 0) {
            close_connection(w, i);
            return;
        }

        c->request_len += (int)n;
        c->request[c->request_len] = '\0';

        if (strstr(c->request, "\r\n\r\n") == NULL) {
            if (c->request_len >= BUF_SIZE - 1) {
                fprintf(stderr, "Error: request too large\n");
                close_connection(w, i);
            }
            return;
        }

        if (parse_request(c) == -1) {
            fprintf(stderr, "Error: cannot parse request\n");
            close_connection(w, i);
            return;
        }

        int hit = cache_get_copy(&g_cache, c->cache_key, &c->response, &c->response_len);
        if (hit == 1) {
            c->response_cap = c->response_len;
            c->from_cache = 1;
            c->bytes_sent = 0;
            c->state = STATE_CLIENT_WRITING;
            return;
        }

        if (start_connect(c) == -1) {
            close_connection(w, i);
        }
        return;
    }

    if (c->state == STATE_CONNECTING && FD_ISSET(c->server_fd, wset)) {
        int err = 0;
        socklen_t elen = sizeof(err);
        if (getsockopt(c->server_fd, SOL_SOCKET, SO_ERROR, &err, &elen) == -1 || err != 0) {
            if (err != 0) {
                fprintf(stderr, "Error connect to %s: %s\n", c->host, strerror(err));
            } else {
                perror("Error getsockopt SO_ERROR");
            }
            close_connection(w, i);
            return;
        }

        if (set_blocking(c->server_fd) == -1) {
            close_connection(w, i);
            return;
        }

        c->bytes_sent = 0;
        c->state = STATE_SERVER_WRITING;
        return;
    }

    if (c->state == STATE_SERVER_WRITING && FD_ISSET(c->server_fd, wset)) {
        while (c->bytes_sent < (size_t)c->request_len) {
            ssize_t n = send(c->server_fd,
                             c->request + c->bytes_sent,
                             (size_t)(c->request_len - (int)c->bytes_sent),
                             0);
            if (n == -1) {
                perror("Error send to server");
                break;
            }
            c->bytes_sent += (size_t)n;
        }

        if (c->bytes_sent < (size_t)c->request_len) {
            close_connection(w, i);
            return;
        }

        c->response_len = 0;
        c->response_cap = 0;
        c->response = NULL;
        c->state = STATE_SERVER_READING;
        return;
    }

    if (c->state == STATE_SERVER_READING && FD_ISSET(c->server_fd, rset)) {
        ssize_t n = recv(c->server_fd, tmp_buf, BUF_SIZE, 0);
        if (n == -1) {
            perror("Error recv from server");
            close_connection(w, i);
            return;
        }

        if (n == 0) {
            shutdown(c->server_fd, SHUT_RDWR);
            close(c->server_fd);
            c->server_fd = -1;

            if (c->response_len > 0) {
                cache_put(&g_cache, c->cache_key, c->response, c->response_len);
            }

            c->bytes_sent = 0;
            c->state = STATE_CLIENT_WRITING;
            return;
        }

        if (append_response(c, tmp_buf, n) == -1) {
            close_connection(w, i);
        }
        return;
    }

    if (c->state == STATE_CLIENT_WRITING && FD_ISSET(c->client_fd, wset)) {
        if (c->response == NULL || c->response_len == 0) {
            close_connection(w, i);
            return;
        }

        ssize_t n = send(c->client_fd,
                         c->response + c->bytes_sent,
                         c->response_len - c->bytes_sent,
                         0);
        if (n == -1) {
            perror("Error send to client");
            close_connection(w, i);
            return;
        }
        c->bytes_sent += (size_t)n;

        if (c->bytes_sent >= c->response_len) {
            c->state = STATE_DONE;
            close_connection(w, i);
        }
        return;
    }
}

void* worker_routine(void* arg) {
    worker_t *w = (worker_t *)arg;
    char *tmp_buf = malloc(BUF_SIZE);
    if (tmp_buf == NULL) {
        perror("Error malloc worker buffer");
        return NULL;
    }

    int pipe_read = w->pipe_fd[0];

    while (g_running) {
        fd_set rset, wset;
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        FD_SET(pipe_read, &rset);
        int maxfd = pipe_read;

        for (int i = 0; i < w->nconns; i++) {
            connection_t *c = &w->conns[i];

            switch (c->state) {
            case STATE_CLIENT_READING:
                FD_SET(c->client_fd, &rset);
                if (c->client_fd > maxfd) maxfd = c->client_fd;
                break;
            case STATE_CONNECTING:
            case STATE_SERVER_WRITING:
                FD_SET(c->server_fd, &wset);
                if (c->server_fd > maxfd) maxfd = c->server_fd;
                break;
            case STATE_SERVER_READING:
                FD_SET(c->server_fd, &rset);
                if (c->server_fd > maxfd) maxfd = c->server_fd;
                break;
            case STATE_CLIENT_WRITING:
                FD_SET(c->client_fd, &wset);
                if (c->client_fd > maxfd) maxfd = c->client_fd;
                break;
            default:
                break;
            }
        }

        int nready = select(maxfd + 1, &rset, &wset, NULL, NULL);
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("Error select in worker");
            break;
        }

        if (FD_ISSET(pipe_read, &rset)) {
            int new_fd;
            ssize_t n = read(pipe_read, &new_fd, sizeof(new_fd));
            if (n == 0) {
                break;
            } else if (n == sizeof(new_fd)) {
                accept_connection(w, new_fd);
            } else if (n == -1) {
                perror("Error read worker pipe");
            }
        }

        for (int i = 0; i < w->nconns; i++) {
            int before = w->nconns;
            handle_connection(w, i, &rset, &wset, tmp_buf);
            if (w->nconns < before) {
                i--;
            }
        }
    }

    for (int i = w->nconns - 1; i >= 0; i--) {
        close_connection(w, i);
    }

    free(tmp_buf);
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

    if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pool_size> [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int pool_size = atoi(argv[1]);
    if (pool_size <= 0) {
        fprintf(stderr, "Error: invalid pool size %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int port = DEFAULT_PORT;
    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Error: invalid port %s\n", argv[2]);
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

    int res;
    if (cache_init(&g_cache) == -1) {
        return EXIT_FAILURE;
    }

    int lfd = create_listen_socket(port);
    if (lfd == -1) {
        cache_destroy(&g_cache);
        return EXIT_FAILURE;
    }

    worker_t *workers = calloc((size_t)pool_size, sizeof(worker_t));
    if (workers == NULL) {
        perror("Error calloc workers");
        shutdown(lfd, SHUT_RDWR);
        close(lfd);
        cache_destroy(&g_cache);
        return EXIT_FAILURE;
    }

    int started = 0;
    for (int i = 0; i < pool_size; i++) {
        worker_t *w = &workers[i];
        w->id = i;
        w->nconns = 0;

        if (pipe(w->pipe_fd) == -1) {
            perror("Error pipe");
            break;
        }

        if ((res = pthread_create(&w->tid, NULL, worker_routine, w)) != 0) {
            fprintf(stderr, "Error pthread_create: %s\n", strerror(res));
            close(w->pipe_fd[0]);
            close(w->pipe_fd[1]);
            break;
        }
        started++;
    }

    if (started == 0) {
        fprintf(stderr, "Error: no workers started\n");
        free(workers);
        shutdown(lfd, SHUT_RDWR);
        close(lfd);
        cache_destroy(&g_cache);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Proxy listening on port %d, %d worker(s)\n", port, started);

    int next_worker = 0;

    while (g_running) {
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(lfd, &rset);

        int nready = select(lfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("Error select in main");
            break;
        }

        if (FD_ISSET(lfd, &rset)) {
            struct sockaddr_in caddr;
            socklen_t clen = sizeof(caddr);
            int cfd = accept(lfd, (struct sockaddr *)&caddr, &clen);
            if (cfd == -1) {
                if (errno == EINTR) continue;
                perror("Error accept");
                continue;
            }

            worker_t *w = &workers[next_worker];
            next_worker = (next_worker + 1) % started;

            ssize_t wr = write(w->pipe_fd[1], &cfd, sizeof(cfd));
            if (wr != sizeof(cfd)) {
                perror("Error dispatch to worker");
                shutdown(cfd, SHUT_RDWR);
                close(cfd);
            }
        }
    }

    for (int i = 0; i < started; i++) {
        close(workers[i].pipe_fd[1]);
    }

    for (int i = 0; i < started; i++) {
        if ((res = pthread_join(workers[i].tid, NULL)) != 0) {
            fprintf(stderr, "Error pthread_join: %s\n", strerror(res));
        }
        close(workers[i].pipe_fd[0]);
    }

    free(workers);

    if (lfd >= 0) {
        shutdown(lfd, SHUT_RDWR);
        close(lfd);
    }

    cache_destroy(&g_cache);
    return EXIT_SUCCESS;
}
