#include <stdio.h>
#include <stdlib.h>
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

#define DEFAULT_PORT   8080
#define MAX_CLIENTS    100
#define BUF_SIZE       65536
#define GROW_SIZE      65536

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

cache_t g_cache;
connection_t g_conns[MAX_CLIENTS];
int g_nconns = 0;

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

void close_connection(int idx) {
    connection_t *c = &g_conns[idx];

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
    if (c->response != NULL && !c->from_cache) {
        free(c->response);
    }
    c->response = NULL;

    g_conns[idx] = g_conns[g_nconns - 1];
    g_nconns--;
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

    cache_init(&g_cache);

    int lfd = create_listen_socket(port);
    if (lfd == -1) {
        cache_destroy(&g_cache);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Proxy listening on port %d\n", port);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        g_conns[i].client_fd = -1;
        g_conns[i].server_fd = -1;
        g_conns[i].response = NULL;
    }

    char tmp_buf[BUF_SIZE];

    while (g_running) {
        fd_set rset, wset;
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        FD_SET(lfd, &rset);
        int maxfd = lfd;

        for (int i = 0; i < g_nconns; i++) {
            connection_t *c = &g_conns[i];

            switch (c->state) {
            case STATE_CLIENT_READING:
                FD_SET(c->client_fd, &rset);
                if (c->client_fd > maxfd) maxfd = c->client_fd;
                break;

            case STATE_CONNECTING:
                FD_SET(c->server_fd, &wset);
                if (c->server_fd > maxfd) maxfd = c->server_fd;
                break;

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
            perror("Error select");
            break;
        }

        if (FD_ISSET(lfd, &rset)) {
            struct sockaddr_in caddr;
            socklen_t clen = sizeof(caddr);
            int cfd = accept(lfd, (struct sockaddr *)&caddr, &clen);
            if (cfd == -1) {
                perror("Error accept");
            } else if (g_nconns >= MAX_CLIENTS) {
                fprintf(stderr, "Error: too many clients, dropping connection\n");
                shutdown(cfd, SHUT_RDWR);
                close(cfd);
            } else {
                {
                    connection_t *c = &g_conns[g_nconns];
                    memset(c, 0, sizeof(*c));
                    c->client_fd = cfd;
                    c->server_fd = -1;
                    c->state = STATE_CLIENT_READING;
                    c->request_len = 0;
                    c->response = NULL;
                    c->response_len = 0;
                    c->response_cap = 0;
                    c->bytes_sent = 0;
                    c->from_cache = 0;
                    g_nconns++;
                }
            }
        }

        for (int i = 0; i < g_nconns; i++) {
            connection_t *c = &g_conns[i];

            if (c->state == STATE_CLIENT_READING && FD_ISSET(c->client_fd, &rset)) {
                ssize_t n = recv(c->client_fd, c->request + c->request_len,
                                 BUF_SIZE - c->request_len - 1, 0);
                if (n <= 0) {
                    close_connection(i);
                    i--;
                    continue;
                }

                c->request_len += (int)n;
                c->request[c->request_len] = '\0';

                if (strstr(c->request, "\r\n\r\n") == NULL) {
                    if (c->request_len >= BUF_SIZE - 1) {
                        fprintf(stderr, "Error: request too large\n");
                        close_connection(i);
                        i--;
                    }
                    continue;
                }

                if (parse_request(c) == -1) {
                    fprintf(stderr, "Error: cannot parse request\n");
                    close_connection(i);
                    i--;
                    continue;
                }

                const cache_entry_t *hit = cache_get(&g_cache, c->cache_key);
                if (hit != NULL) {
                    c->response = hit->data;
                    c->response_len = hit->data_len;
                    c->response_cap = hit->data_len;
                    c->from_cache = 1;
                    c->bytes_sent = 0;
                    c->state = STATE_CLIENT_WRITING;
                    continue;
                }

                if (start_connect(c) == -1) {
                    close_connection(i);
                    i--;
                }
                continue;
            }

            if (c->state == STATE_CONNECTING && FD_ISSET(c->server_fd, &wset)) {
                int err = 0;
                socklen_t elen = sizeof(err);
                if (getsockopt(c->server_fd, SOL_SOCKET, SO_ERROR, &err, &elen) == -1 || err != 0) {
                    if (err != 0) {
                        fprintf(stderr, "Error connect to %s: %s\n", c->host, strerror(err));
                    } else {
                        perror("Error getsockopt SO_ERROR");
                    }
                    close_connection(i);
                    i--;
                    continue;
                }

                if (set_blocking(c->server_fd) == -1) {
                    close_connection(i);
                    i--;
                    continue;
                }

                c->bytes_sent = 0;
                c->state = STATE_SERVER_WRITING;
                continue;
            }

            if (c->state == STATE_SERVER_WRITING && FD_ISSET(c->server_fd, &wset)) {
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
                    close_connection(i);
                    i--;
                    continue;
                }

                c->response_len = 0;
                c->response_cap = 0;
                c->response = NULL;
                c->state = STATE_SERVER_READING;
                continue;
            }

            if (c->state == STATE_SERVER_READING && FD_ISSET(c->server_fd, &rset)) {
                ssize_t n = recv(c->server_fd, tmp_buf, sizeof(tmp_buf), 0);
                if (n == -1) {
                    perror("Error recv from server");
                    close_connection(i);
                    i--;
                    continue;
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
                    continue;
                }

                if (append_response(c, tmp_buf, n) == -1) {
                    close_connection(i);
                    i--;
                }
                continue;
            }

            if (c->state == STATE_CLIENT_WRITING && FD_ISSET(c->client_fd, &wset)) {
                if (c->response == NULL || c->response_len == 0) {
                    close_connection(i);
                    i--;
                    continue;
                }

                ssize_t n = send(c->client_fd,
                                 c->response + c->bytes_sent,
                                 c->response_len - c->bytes_sent,
                                 0);
                if (n == -1) {
                    perror("Error send to client");
                    close_connection(i);
                    i--;
                    continue;
                }
                c->bytes_sent += (size_t)n;

                if (c->bytes_sent >= c->response_len) {
                    c->state = STATE_DONE;
                    close_connection(i);
                    i--;
                }
                continue;
            }
        }
    }

    for (int i = g_nconns - 1; i >= 0; i--) {
        close_connection(i);
    }

    if (lfd >= 0) {
        shutdown(lfd, SHUT_RDWR);
        close(lfd);
    }

    cache_destroy(&g_cache);
    return EXIT_SUCCESS;
}
