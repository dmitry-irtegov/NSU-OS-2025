#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_CONN 510
#define BUF_SIZE 4096
#define MAX_PFDS (1 + MAX_CONN * 2)

typedef struct {
    int client_fd;
    int server_fd;

    char c2s_buf[BUF_SIZE];
    size_t c2s_len;
    size_t c2s_off;

    char s2c_buf[BUF_SIZE];
    size_t s2c_len;
    size_t s2c_off;
} connection_t;

static connection_t conns[MAX_CONN];
static int active_count = 0;
static volatile sig_atomic_t stop_requested = 0;

static void sigint_handler(int signo) {
    (void)signo;
    stop_requested = 1;
}

static void init_connections(void) {
    for (int i = 0; i < MAX_CONN; i++) {
        conns[i].client_fd = -1;
        conns[i].server_fd = -1;
        conns[i].c2s_len = 0;
        conns[i].c2s_off = 0;
        conns[i].s2c_len = 0;
        conns[i].s2c_off = 0;
    }
}

static void close_connection(int i) {
    if (conns[i].client_fd != -1) {
        close(conns[i].client_fd);
        conns[i].client_fd = -1;
    }

    if (conns[i].server_fd != -1) {
        close(conns[i].server_fd);
        conns[i].server_fd = -1;
    }

    conns[i].c2s_len = 0;
    conns[i].c2s_off = 0;
    conns[i].s2c_len = 0;
    conns[i].s2c_off = 0;

    if (active_count > 0) {
        active_count--;
    }
}

static void close_all_connections(void) {
    for (int i = 0; i < MAX_CONN; i++) {
        if (conns[i].client_fd != -1 || conns[i].server_fd != -1) {
            close_connection(i);
        }
    }
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_CONN; i++) {
        if (conns[i].client_fd == -1 && conns[i].server_fd == -1) {
            return i;
        }
    }

    return -1;
}

static int create_listener(const char *port) {
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *rp;
    int listen_fd = -1;
    int opt = 1;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    ret = getaddrinfo(NULL, port, &hints, &res);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1) {
            continue;
        }

        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(listen_fd, 512) == 0) {
                break;
            }
        }

        close(listen_fd);
        listen_fd = -1;
    }

    freeaddrinfo(res);

    return listen_fd;
}

static int connect_to_target(struct addrinfo *target) {
    struct addrinfo *rp;
    int fd;

    for (rp = target; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            return fd;
        }

        close(fd);
    }

    return -1;
}

static int read_to_buffer(int fd, char *buf, size_t *len, size_t *off) {
    ssize_t n;

    do {
        n = read(fd, buf, BUF_SIZE);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        return -1;
    }

    if (n == 0) {
        return -1;
    }

    *len = (size_t)n;
    *off = 0;

    return 0;
}

static int write_from_buffer(int fd, char *buf, size_t *len, size_t *off) {
    ssize_t n;

    do {
        n = write(fd, buf + *off, *len - *off);
    } while (n < 0 && errno == EINTR);

    if (n < 0) {
        return -1;
    }

    if (n == 0) {
        return -1;
    }

    *off += (size_t)n;

    if (*off == *len) {
        *off = 0;
        *len = 0;
    }

    return 0;
}

int main(int argc, char **argv) {
    const char *listen_port;
    const char *target_host;
    const char *target_port;

    struct addrinfo hints;
    struct addrinfo *target;

    struct pollfd pfds[MAX_PFDS];
    int map_conn[MAX_PFDS];
    int map_side[MAX_PFDS];

    int listen_fd;
    int ret;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <listen_port> <target_host> <target_port>\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    listen_port = argv[1];
    target_host = argv[2];
    target_port = argv[3];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo(target_host, target_port, &hints, &target);
    if (ret != 0) {
        fprintf(stderr, "target getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        freeaddrinfo(target);
        return 1;
    }

    listen_fd = create_listener(listen_port);
    if (listen_fd == -1) {
        perror("create_listener");
        freeaddrinfo(target);
        return 1;
    }

    init_connections();

    printf("proxy listening on port %s, forwarding to %s:%s\n",
           listen_port, target_host, target_port);
    fflush(stdout);

    while (!stop_requested) {
        int nfds = 0;

        pfds[nfds].fd = listen_fd;
        pfds[nfds].events = active_count < MAX_CONN ? POLLIN : 0;
        pfds[nfds].revents = 0;
        map_conn[nfds] = -1;
        map_side[nfds] = -1;
        nfds++;

        for (int i = 0; i < MAX_CONN; i++) {
            if (conns[i].client_fd == -1) {
                continue;
            }

            pfds[nfds].fd = conns[i].client_fd;
            pfds[nfds].events = 0;

            if (conns[i].c2s_len == 0) {
                pfds[nfds].events |= POLLIN;
            }

            if (conns[i].s2c_len > conns[i].s2c_off) {
                pfds[nfds].events |= POLLOUT;
            }

            pfds[nfds].revents = 0;
            map_conn[nfds] = i;
            map_side[nfds] = 0;
            nfds++;

            pfds[nfds].fd = conns[i].server_fd;
            pfds[nfds].events = 0;

            if (conns[i].s2c_len == 0) {
                pfds[nfds].events |= POLLIN;
            }

            if (conns[i].c2s_len > conns[i].c2s_off) {
                pfds[nfds].events |= POLLOUT;
            }

            pfds[nfds].revents = 0;
            map_conn[nfds] = i;
            map_side[nfds] = 1;
            nfds++;
        }

        ret = poll(pfds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            break;
        }

        if (pfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "listen socket error\n");
            break;
        }

        if (pfds[0].revents & POLLIN) {
            int client_fd;
            int server_fd;
            int slot;

            do {
                client_fd = accept(listen_fd, NULL, NULL);
            } while (client_fd == -1 && errno == EINTR);

            if (client_fd != -1) {
                slot = find_free_slot();

                if (slot == -1) {
                    close(client_fd);
                } else {
                    server_fd = connect_to_target(target);

                    if (server_fd == -1) {
                        close(client_fd);
                    } else {
                        conns[slot].client_fd = client_fd;
                        conns[slot].server_fd = server_fd;
                        conns[slot].c2s_len = 0;
                        conns[slot].c2s_off = 0;
                        conns[slot].s2c_len = 0;
                        conns[slot].s2c_off = 0;
                        active_count++;
                    }
                }
            }
        }

        for (int p = 1; p < nfds; p++) {
            int i = map_conn[p];
            int side = map_side[p];
            short re = pfds[p].revents;

            if (i < 0 || conns[i].client_fd == -1) {
                continue;
            }

            if (re & (POLLERR | POLLHUP | POLLNVAL)) {
                close_connection(i);
                continue;
            }

            if (side == 0) {
                if ((re & POLLIN) && conns[i].c2s_len == 0) {
                    if (read_to_buffer(conns[i].client_fd,
                                       conns[i].c2s_buf,
                                       &conns[i].c2s_len,
                                       &conns[i].c2s_off) != 0) {
                        close_connection(i);
                        continue;
                    }
                }

                if ((re & POLLOUT) && conns[i].s2c_len > conns[i].s2c_off) {
                    if (write_from_buffer(conns[i].client_fd,
                                          conns[i].s2c_buf,
                                          &conns[i].s2c_len,
                                          &conns[i].s2c_off) != 0) {
                        close_connection(i);
                        continue;
                    }
                }
            } else {
                if ((re & POLLIN) && conns[i].s2c_len == 0) {
                    if (read_to_buffer(conns[i].server_fd,
                                       conns[i].s2c_buf,
                                       &conns[i].s2c_len,
                                       &conns[i].s2c_off) != 0) {
                        close_connection(i);
                        continue;
                    }
                }

                if ((re & POLLOUT) && conns[i].c2s_len > conns[i].c2s_off) {
                    if (write_from_buffer(conns[i].server_fd,
                                          conns[i].c2s_buf,
                                          &conns[i].c2s_len,
                                          &conns[i].c2s_off) != 0) {
                        close_connection(i);
                        continue;
                    }
                }
            }
        }
    }

    close_all_connections();
    close(listen_fd);
    freeaddrinfo(target);

    return stop_requested ? 1 : 0;
}