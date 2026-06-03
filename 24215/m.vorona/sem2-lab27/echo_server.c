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

#define MAX_CLIENTS 600
#define BUF_SIZE 4096
#define MAX_PFDS (1 + MAX_CLIENTS)

typedef struct {
    int fd;
    char buf[BUF_SIZE];
    size_t len;
    size_t off;
} client_t;

static client_t clients[MAX_CLIENTS];

static void init_clients(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].len = 0;
        clients[i].off = 0;
    }
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            return i;
        }
    }

    return -1;
}

static void close_client(int i) {
    if (clients[i].fd != -1) {
        close(clients[i].fd);
        clients[i].fd = -1;
    }

    clients[i].len = 0;
    clients[i].off = 0;
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

int main(int argc, char **argv) {
    struct pollfd pfds[MAX_PFDS];
    int map[MAX_PFDS];
    int listen_fd;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    listen_fd = create_listener(argv[1]);
    if (listen_fd == -1) {
        perror("create_listener");
        return 1;
    }

    init_clients();

    printf("echo server listening on port %s\n", argv[1]);
    fflush(stdout);

    while (1) {
        int nfds = 0;

        pfds[nfds].fd = listen_fd;
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        map[nfds] = -1;
        nfds++;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd == -1) {
                continue;
            }

            pfds[nfds].fd = clients[i].fd;
            pfds[nfds].events = 0;

            if (clients[i].len == 0) {
                pfds[nfds].events |= POLLIN;
            }

            if (clients[i].len > clients[i].off) {
                pfds[nfds].events |= POLLOUT;
            }

            pfds[nfds].revents = 0;
            map[nfds] = i;
            nfds++;
        }

        if (poll(pfds, nfds, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            break;
        }

        if (pfds[0].revents & POLLIN) {
            int fd = accept(listen_fd, NULL, NULL);

            if (fd != -1) {
                int slot = find_free_slot();

                if (slot == -1) {
                    close(fd);
                } else {
                    clients[slot].fd = fd;
                    clients[slot].len = 0;
                    clients[slot].off = 0;
                }
            }
        }

        for (int p = 1; p < nfds; p++) {
            int i = map[p];
            short re = pfds[p].revents;

            if (i < 0 || clients[i].fd == -1) {
                continue;
            }

            if (re & (POLLERR | POLLHUP | POLLNVAL)) {
                close_client(i);
                continue;
            }

            if ((re & POLLIN) && clients[i].len == 0) {
                ssize_t n = read(clients[i].fd, clients[i].buf, BUF_SIZE);

                if (n <= 0) {
                    close_client(i);
                    continue;
                }

                clients[i].len = (size_t)n;
                clients[i].off = 0;
            }

            if ((re & POLLOUT) && clients[i].len > clients[i].off) {
                ssize_t n = write(clients[i].fd,
                                  clients[i].buf + clients[i].off,
                                  clients[i].len - clients[i].off);

                if (n <= 0) {
                    close_client(i);
                    continue;
                }

                clients[i].off += (size_t)n;

                if (clients[i].off == clients[i].len) {
                    clients[i].off = 0;
                    clients[i].len = 0;
                }
            }
        }
    }

    close(listen_fd);
    return 1;
}