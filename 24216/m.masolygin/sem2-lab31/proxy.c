#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.h"
#include "http.h"

volatile sig_atomic_t server_running = 1;

#define MAX_CONNS 100
#define BUF_SIZE 8192
#define INIT_WBUF_SIZE 4096

typedef enum conn_state {
    S_READ_REQ,
    S_CONNECTING,
    S_WRITE_REQ,
    S_READ_RESP,
    S_WRITE_RESP,
    S_DONE
} conn_state_t;

typedef struct conn {
    int client_fd;
    int server_fd;
    char rbuf[BUF_SIZE];
    size_t rbuf_len;
    size_t rbuf_sent;

    char* wbuf;
    size_t wbuf_cap;
    size_t wbuf_len;
    size_t wbuf_sent;

    size_t body_expected;
    size_t header_end;
    size_t body_got;
    int server_close;
    int server_port;

    char url[512];
    conn_state_t state;

    int pfd_client;
    int pfd_server;
} conn_t;

cache_t* g_cache;
conn_t g_conns[MAX_CONNS];

void conn_reset(conn_t* c) {
    c->client_fd = -1;
    c->server_fd = -1;
    c->rbuf_len = 0;
    c->rbuf_sent = 0;
    c->wbuf = NULL;
    c->wbuf_cap = 0;
    c->wbuf_len = 0;
    c->wbuf_sent = 0;
    c->body_expected = 0;
    c->header_end = 0;
    c->body_got = 0;
    c->server_close = 0;
    c->server_port = 80;
    c->url[0] = '\0';
    c->state = S_READ_REQ;
    c->pfd_client = -1;
    c->pfd_server = -1;
}

void conn_init_all() {
    for (int i = 0; i < MAX_CONNS; i++) {
        conn_reset(&g_conns[i]);
    }
}

void resp_free(conn_t* c) {
    if (c->wbuf) {
        free(c->wbuf);
        c->wbuf = NULL;
        c->wbuf_cap = 0;
        c->wbuf_len = 0;
    }
}

void conn_close(int idx) {
    if (g_conns[idx].client_fd != -1) {
        shutdown(g_conns[idx].client_fd, SHUT_RDWR);
        close(g_conns[idx].client_fd);
        g_conns[idx].client_fd = -1;
    }
    if (g_conns[idx].server_fd != -1) {
        shutdown(g_conns[idx].server_fd, SHUT_RDWR);
        close(g_conns[idx].server_fd);
        g_conns[idx].server_fd = -1;
    }
    resp_free(&g_conns[idx]);
}

int conn_alloc(int client_fd) {
    for (int i = 0; i < MAX_CONNS; i++) {
        if (g_conns[i].client_fd == -1) {
            conn_reset(&g_conns[i]);
            g_conns[i].client_fd = client_fd;
            return i;
        }
    }
    return -1;
}

int resp_grow(conn_t* c, size_t needed) {
    if (c->wbuf_cap >= needed) return 0;

    size_t ncap = c->wbuf_cap ? c->wbuf_cap * 2 : INIT_WBUF_SIZE;
    while (ncap < needed) ncap *= 2;

    char* nb = realloc(c->wbuf, ncap);
    if (!nb) return -1;

    c->wbuf = nb;
    c->wbuf_cap = ncap;
    return 0;
}

int is_2xx(const char* buf) {
    const char* sp = memchr(buf, ' ', 12);
    if (!sp) return 0;
    int code = atoi(sp + 1);
    return code >= 200 && code < 300;
}

void try_cache(conn_t* c) {
    if (c->wbuf_len <= 12) return;
    if (strncmp(c->wbuf, "HTTP/", 5) != 0) return;
    if (!is_2xx(c->wbuf)) return;

    char* key_copy = strdup(c->url);
    char* body_copy = malloc(c->wbuf_len);
    if (key_copy && body_copy) {
        memcpy(body_copy, c->wbuf, c->wbuf_len);
        cache_entry_t* entry =
            cache_entry_new(body_copy, c->wbuf_len, key_copy);
        if (entry) {
            cache_entry_t* removed = cache_put(g_cache, entry);
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

void sig_handler(int sig) {
    (void)sig;
    server_running = 0;
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
    conn_init_all();

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

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, MAX_CONNS) < 0) {
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
        struct pollfd nfds_arr[MAX_CONNS * 2 + 1];
        int nfds = 0;

        nfds_arr[nfds].fd = listen_fd;
        nfds_arr[nfds].events = POLLIN;
        nfds_arr[nfds].revents = 0;
        int listen_idx = nfds;
        nfds++;

        for (int i = 0; i < MAX_CONNS; i++) {
            if (g_conns[i].client_fd == -1) continue;
            conn_t* c = &g_conns[i];
            c->pfd_client = -1;
            c->pfd_server = -1;

            switch (c->state) {
                case S_READ_REQ:
                    c->pfd_client = nfds;
                    nfds_arr[nfds].fd = c->client_fd;
                    nfds_arr[nfds].events = POLLIN;
                    nfds_arr[nfds].revents = 0;
                    nfds++;
                    break;
                case S_CONNECTING:
                case S_WRITE_REQ:
                    c->pfd_server = nfds;
                    nfds_arr[nfds].fd = c->server_fd;
                    nfds_arr[nfds].events = POLLOUT;
                    nfds_arr[nfds].revents = 0;
                    nfds++;
                    break;
                case S_READ_RESP:
                    if (!c->server_close && c->server_fd != -1) {
                        c->pfd_server = nfds;
                        nfds_arr[nfds].fd = c->server_fd;
                        nfds_arr[nfds].events = POLLIN;
                        nfds_arr[nfds].revents = 0;
                        nfds++;
                    }
                    if (c->wbuf_len > c->wbuf_sent) {
                        c->pfd_client = nfds;
                        nfds_arr[nfds].fd = c->client_fd;
                        nfds_arr[nfds].events = POLLOUT;
                        nfds_arr[nfds].revents = 0;
                        nfds++;
                    }
                    break;
                case S_WRITE_RESP:
                    c->pfd_client = nfds;
                    nfds_arr[nfds].fd = c->client_fd;
                    nfds_arr[nfds].events = POLLOUT;
                    nfds_arr[nfds].revents = 0;
                    nfds++;
                    break;
                case S_DONE:
                    break;
            }
        }

        int activity = poll(nfds_arr, nfds, -1);
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (nfds_arr[listen_idx].revents & (POLLIN | POLLERR)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int client_fd =
                accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);

            if (client_fd >= 0) {
                if (conn_alloc(client_fd) < 0) {
                    http_send_error(client_fd, 503, "Service Unavailable");
                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < MAX_CONNS; i++) {
            if (g_conns[i].client_fd == -1) continue;
            conn_t* c = &g_conns[i];

            int cli_ev =
                (c->pfd_client >= 0) ? nfds_arr[c->pfd_client].revents : 0;
            int srv_ev =
                (c->pfd_server >= 0) ? nfds_arr[c->pfd_server].revents : 0;

            if ((cli_ev & POLLERR) || (srv_ev & POLLERR)) {
                conn_close(i);
                continue;
            }

            switch (c->state) {
                case S_READ_REQ:
                    if (cli_ev & (POLLIN | POLLHUP)) {
                        if (c->rbuf_len >= BUF_SIZE - 1) {
                            http_send_error(c->client_fd, 413,
                                            "Request Entity Too Large");
                            conn_close(i);
                            continue;
                        }
                        ssize_t n = recv(c->client_fd, c->rbuf + c->rbuf_len,
                                         BUF_SIZE - c->rbuf_len - 1, 0);
                        if (n == 0) {
                            conn_close(i);
                            continue;
                        }
                        if (n < 0) {
                            conn_close(i);
                            continue;
                        }

                        c->rbuf_len += n;
                        c->rbuf[c->rbuf_len] = '\0';

                        if (strstr(c->rbuf, "\r\n\r\n") ||
                            strstr(c->rbuf, "\n\n")) {
                            http_request_t hreq;
                            if (http_req_parse(c->rbuf, &hreq) != 0) {
                                http_send_error(c->client_fd, 400,
                                                "Bad Request");
                                conn_close(i);
                                continue;
                            }

                            c->server_port = hreq.port;
                            snprintf(c->url, sizeof(c->url), "http://%s:%d%s",
                                     hreq.host, hreq.port, hreq.path);

                            cache_entry_t* cached = cache_get(g_cache, c->url);
                            if (cached) {
                                if (resp_grow(c, cached->body_len) == 0) {
                                    memcpy(c->wbuf, cached->body,
                                           cached->body_len);
                                    c->wbuf_len = cached->body_len;
                                    c->wbuf_sent = 0;
                                    c->state = S_WRITE_RESP;
                                } else {
                                    conn_close(i);
                                }
                                continue;
                            }
                            if (hreq.port == 80) {
                                c->rbuf_len =
                                    (size_t)snprintf(c->rbuf, BUF_SIZE,
                                                     "GET %s HTTP/1.0\r\n"
                                                     "Host: %s\r\n"
                                                     "Connection: close\r\n"
                                                     "\r\n",
                                                     hreq.path, hreq.host);
                            } else {
                                c->rbuf_len = (size_t)snprintf(
                                    c->rbuf, BUF_SIZE,
                                    "GET %s HTTP/1.0\r\n"
                                    "Host: %s:%d\r\n"
                                    "Connection: close\r\n"
                                    "\r\n",
                                    hreq.path, hreq.host, hreq.port);
                            }
                            c->rbuf_sent = 0;
                            c->server_fd = tcp_connect(hreq.host, hreq.port);
                            if (c->server_fd < 0) {
                                http_send_error(c->client_fd, 502,
                                                "Bad Gateway");
                                conn_close(i);
                                continue;
                            }

                            c->state = S_CONNECTING;
                        }
                    }
                    break;

                case S_CONNECTING:
                    if (srv_ev & (POLLOUT | POLLERR)) {
                        int err = 0;
                        socklen_t slen = sizeof(err);
                        if (getsockopt(c->server_fd, SOL_SOCKET, SO_ERROR, &err,
                                       &slen) < 0 ||
                            err != 0) {
                            fprintf(stderr, "connect to %s: %s\n", c->url,
                                    strerror(err));
                            http_send_error(c->client_fd, 502, "Bad Gateway");
                            conn_close(i);
                            continue;
                        }
                        tcp_set_blocking(c->server_fd);
                        c->state = S_WRITE_REQ;
                    }
                    break;

                case S_WRITE_REQ:
                    if (srv_ev & POLLOUT) {
                        ssize_t sent =
                            send(c->server_fd, c->rbuf + c->rbuf_sent,
                                 c->rbuf_len - c->rbuf_sent, 0);
                        if (sent < 0) {
                            conn_close(i);
                            continue;
                        }
                        c->rbuf_sent += (size_t)sent;
                        if (c->rbuf_sent >= c->rbuf_len) {
                            c->state = S_READ_RESP;
                            c->wbuf_len = 0;
                            c->wbuf_sent = 0;
                            if (resp_grow(c, INIT_WBUF_SIZE) < 0) {
                                conn_close(i);
                                continue;
                            }
                        }
                    }
                    break;

                case S_READ_RESP:
                    if (!c->server_close && c->server_fd != -1 &&
                        (srv_ev & (POLLIN | POLLHUP))) {
                        if (resp_grow(c, c->wbuf_len + BUF_SIZE) < 0) {
                            conn_close(i);
                            continue;
                        }
                        ssize_t n = recv(c->server_fd, c->wbuf + c->wbuf_len,
                                         BUF_SIZE, 0);
                        if (n < 0) {
                            conn_close(i);
                            continue;
                        } else if (n == 0) {
                            shutdown(c->server_fd, SHUT_RDWR);
                            close(c->server_fd);
                            c->server_fd = -1;
                            c->server_close = 1;
                            try_cache(c);
                        } else {
                            c->wbuf_len += n;
                            if (c->header_end == 0) {
                                ssize_t he = http_resp_parse_headers(
                                    c->wbuf, c->wbuf_len, &c->body_expected);
                                if (he >= 0) {
                                    c->header_end = (size_t)he;
                                    char conn_hdr[64];
                                    if (http_header_find(c->wbuf, "Connection",
                                                         conn_hdr,
                                                         sizeof(conn_hdr)) &&
                                        strcasecmp(conn_hdr, "close") == 0) {
                                        c->server_close = 1;
                                    }
                                }
                            }
                            if (c->header_end > 0 && c->body_expected > 0) {
                                c->body_got = c->wbuf_len - c->header_end;
                                if (c->body_got >= c->body_expected) {
                                    c->server_close = 1;
                                    try_cache(c);
                                    if (c->server_fd != -1) {
                                        shutdown(c->server_fd, SHUT_RDWR);
                                        close(c->server_fd);
                                        c->server_fd = -1;
                                    }
                                }
                            }
                        }
                    }

                    if (c->wbuf_sent < c->wbuf_len && (cli_ev & POLLOUT)) {
                        ssize_t sent =
                            send(c->client_fd, c->wbuf + c->wbuf_sent,
                                 c->wbuf_len - c->wbuf_sent, 0);
                        if (sent < 0) {
                            conn_close(i);
                            continue;
                        }
                        c->wbuf_sent += (size_t)sent;
                    }

                    if (c->server_close && c->wbuf_sent >= c->wbuf_len) {
                        conn_close(i);
                    }
                    break;

                case S_WRITE_RESP:
                    if (cli_ev & POLLOUT) {
                        ssize_t sent =
                            send(c->client_fd, c->wbuf + c->wbuf_sent,
                                 c->wbuf_len - c->wbuf_sent, 0);
                        if (sent < 0) {
                            conn_close(i);
                            continue;
                        }
                        c->wbuf_sent += (size_t)sent;
                        if (c->wbuf_sent >= c->wbuf_len) {
                            conn_close(i);
                        }
                    }
                    break;

                case S_DONE:
                    break;
            }
        }
    }

    for (int i = 0; i < MAX_CONNS; i++) {
        if (g_conns[i].client_fd != -1) conn_close(i);
    }
    shutdown(listen_fd, SHUT_RDWR);
    close(listen_fd);
    cache_free(g_cache);
    return 0;
}
