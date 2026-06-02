#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.h"
#include "http.h"

volatile sig_atomic_t server_running = 1;

#define BUF_SIZE 8192
#define INIT_WBUF_SIZE 4096

cache_t* g_cache;
int g_listen_fd = -1;

int resp_grow(char** wbuf, size_t* cap, size_t needed) {
    if (*cap >= needed) return 0;

    size_t ncap = *cap ? *cap * 2 : INIT_WBUF_SIZE;
    while (ncap < needed) ncap *= 2;

    char* nb = realloc(*wbuf, ncap);
    if (!nb) return -1;

    *wbuf = nb;
    *cap = ncap;
    return 0;
}

int is_2xx(const char* buf) {
    const char* sp = memchr(buf, ' ', 12);
    if (!sp) return 0;
    int code = atoi(sp + 1);
    return code >= 200 && code < 300;
}

int try_cache_hit(const char* url, int client_fd) {
    char* local_copy = NULL;
    size_t local_len = 0;

    pthread_mutex_lock(&g_cache->mutex);
    cache_entry_t* hit = cache_get(g_cache, (char*)url);
    if (hit) {
        local_copy = malloc(hit->body_len);
        if (local_copy) {
            memcpy(local_copy, hit->body, hit->body_len);
            local_len = hit->body_len;
        }
    }
    pthread_mutex_unlock(&g_cache->mutex);

    if (!local_copy) return 0;

    size_t sent_total = 0;
    while (sent_total < local_len) {
        ssize_t sent =
            send(client_fd, local_copy + sent_total, local_len - sent_total, 0);
        if (sent <= 0) break;
        sent_total += (size_t)sent;
    }
    free(local_copy);
    return 1;
}

void try_cache(const char* url, const char* wbuf, size_t wbuf_len) {
    if (wbuf_len <= 12) return;
    if (strncmp(wbuf, "HTTP/", 5) != 0) return;
    if (!is_2xx(wbuf)) return;

    char* key_copy = strdup(url);
    char* body_copy = malloc(wbuf_len);
    if (key_copy && body_copy) {
        memcpy(body_copy, wbuf, wbuf_len);
        cache_entry_t* entry = cache_entry_new(body_copy, wbuf_len, key_copy);
        if (entry) {
            pthread_mutex_lock(&g_cache->mutex);
            cache_entry_t* removed = cache_put(g_cache, entry);
            pthread_mutex_unlock(&g_cache->mutex);
            if (removed) cache_entry_free(removed);
        } else {
            free(key_copy);
            free(body_copy);
        }
    } else {
        free(key_copy);
        free(body_copy);
    }
}

void* handle_client(void* arg) {
    int client_fd = (int)(long)arg;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    char rbuf[BUF_SIZE];
    size_t rbuf_len = 0;

    while (rbuf_len < BUF_SIZE - 1) {
        ssize_t n =
            recv(client_fd, rbuf + rbuf_len, BUF_SIZE - rbuf_len - 1, 0);
        if (n <= 0) {
            close(client_fd);
            pthread_exit(NULL);
        }
        rbuf_len += (size_t)n;
        rbuf[rbuf_len] = '\0';
        if (strstr(rbuf, "\r\n\r\n") || strstr(rbuf, "\n\n")) break;
    }

    if (!strstr(rbuf, "\r\n\r\n") && !strstr(rbuf, "\n\n")) {
        http_send_error(client_fd, 413, "Request Entity Too Large");
        close(client_fd);
        pthread_exit(NULL);
    }

    http_request_t hreq;
    if (http_req_parse(rbuf, &hreq) != 0) {
        http_send_error(client_fd, 400, "Bad Request");
        close(client_fd);
        pthread_exit(NULL);
    }

    char url[512];
    snprintf(url, sizeof(url), "http://%s:%d%s", hreq.host, hreq.port,
             hreq.path);

    if (try_cache_hit(url, client_fd)) {
        close(client_fd);
        pthread_exit(NULL);
    }

    int server_fd = tcp_connect(hreq.host, hreq.port);
    if (server_fd < 0) {
        http_send_error(client_fd, 502, "Bad Gateway");
        close(client_fd);
        pthread_exit(NULL);
    }

    char sreq[BUF_SIZE];
    size_t sreq_len;
    if (hreq.port == 80) {
        sreq_len = (size_t)snprintf(sreq, sizeof(sreq),
                                    "GET %s HTTP/1.0\r\n"
                                    "Host: %s\r\n"
                                    "Connection: close\r\n"
                                    "\r\n",
                                    hreq.path, hreq.host);
    } else {
        sreq_len = (size_t)snprintf(sreq, sizeof(sreq),
                                    "GET %s HTTP/1.0\r\n"
                                    "Host: %s:%d\r\n"
                                    "Connection: close\r\n"
                                    "\r\n",
                                    hreq.path, hreq.host, hreq.port);
    }

    size_t req_sent = 0;
    while (req_sent < sreq_len) {
        ssize_t s = send(server_fd, sreq + req_sent, sreq_len - req_sent, 0);
        if (s <= 0) {
            close(server_fd);
            close(client_fd);
            pthread_exit(NULL);
        }
        req_sent += (size_t)s;
    }

    char* wbuf = NULL;
    size_t wbuf_cap = 0;
    size_t wbuf_len = 0;
    size_t header_end = 0;
    size_t body_expected = 0;
    int server_close = 0;

    while (1) {
        if (resp_grow(&wbuf, &wbuf_cap, wbuf_len + BUF_SIZE) < 0) break;

        ssize_t n = recv(server_fd, wbuf + wbuf_len, BUF_SIZE, 0);
        if (n <= 0) break;

        ssize_t sent_total = 0;
        while (sent_total < n) {
            ssize_t s = send(client_fd, wbuf + wbuf_len + sent_total,
                             (size_t)(n - sent_total), 0);
            if (s <= 0) {
                sent_total = -1;
                break;
            }
            sent_total += s;
        }
        wbuf_len += (size_t)n;

        if (sent_total < 0) {
            server_close = 1;
            break;
        }

        if (header_end == 0) {
            ssize_t he =
                http_resp_parse_headers(wbuf, wbuf_len, &body_expected);
            if (he >= 0) {
                header_end = (size_t)he;
                char conn_hdr[64];
                if (http_header_find(wbuf, "Connection", conn_hdr,
                                     sizeof(conn_hdr)) &&
                    strcasecmp(conn_hdr, "close") == 0) {
                    server_close = 1;
                }
                if (body_expected == 0 && !server_close &&
                    strncmp(wbuf, "HTTP/1.0", 8) == 0) {
                    server_close = 1;
                }
            }
        }

        if (header_end > 0 && body_expected > 0) {
            size_t body_got = wbuf_len - header_end;
            if (body_got >= body_expected) break;
        }
    }

    if (!server_close) {
        try_cache(url, wbuf, wbuf_len);
    }

    free(wbuf);
    shutdown(server_fd, SHUT_RDWR);
    shutdown(client_fd, SHUT_RDWR);
    close(server_fd);
    close(client_fd);
    pthread_exit(NULL);
}

void sig_handler(int sig) {
    (void)sig;
    server_running = 0;
    if (g_listen_fd != -1) {
        shutdown(g_listen_fd, SHUT_RDWR);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    g_cache = cache_new(10);
    if (!g_cache) {
        perror("cache_new");
        return 1;
    }

    g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(g_listen_fd, 128) < 0) {
        perror("listen");
        return 1;
    }

    fprintf(stderr, "Proxy listening on port %d\n", port);

    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    while (server_running) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd =
            accept(g_listen_fd, (struct sockaddr*)&cli_addr, &cli_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            if (!server_running) break;
            perror("accept");
            continue;
        }

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        sigset_t blocked, oldset;
        sigemptyset(&blocked);
        sigaddset(&blocked, SIGINT);
        sigaddset(&blocked, SIGTERM);
        sigaddset(&blocked, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &blocked, &oldset);

        int err =
            pthread_create(&tid, &attr, handle_client, (void*)(long)client_fd);

        pthread_sigmask(SIG_SETMASK, &oldset, NULL);
        pthread_attr_destroy(&attr);

        if (err != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            http_send_error(client_fd, 503, "Service Unavailable");
            close(client_fd);
        }
    }

    if (g_listen_fd != -1) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }
    cache_free(g_cache);
    return 0;
}
