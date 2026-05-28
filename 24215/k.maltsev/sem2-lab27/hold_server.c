#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 1020
#define BUF_SIZE 4096

static void die(const char *msg)
{
    perror(msg);
    exit(1);
}

static int create_listener(const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *rp;
    int fd;
    int yes = 1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    rc = getaddrinfo(NULL, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        exit(1);
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(fd, SOMAXCONN) == 0) {
                freeaddrinfo(res);
                return fd;
            }
        }

        close(fd);
    }

    freeaddrinfo(res);
    fprintf(stderr, "cannot listen on port %s\n", port);
    exit(1);
}

int main(int argc, char **argv)
{
    struct pollfd fds[MAX_CLIENTS + 1];
    nfds_t nfds;
    int listen_fd;
    int client_fd;
    char buf[BUF_SIZE];
    ssize_t n;
    int i;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    listen_fd = create_listener(argv[1]);

    nfds = 1;
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    printf("hold_server: listening on port %s\n", argv[1]);

    for (;;) {
        if (poll(fds, nfds, -1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            die("poll");
        }

        if (fds[0].revents & POLLIN) {
            client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd == -1) {
                perror("accept");
            } else if (nfds >= MAX_CLIENTS + 1) {
                close(client_fd);
            } else {
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                fds[nfds].revents = 0;
                ++nfds;

                printf("accepted connection, active=%lu\n", (unsigned long)(nfds - 1));
                fflush(stdout);
            }
        }

        for (i = 1; i < (int)nfds; ++i) {
            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                close(fds[i].fd);
                fds[i] = fds[nfds - 1];
                --nfds;
                --i;
                continue;
            }

            if (fds[i].revents & POLLIN) {
                n = read(fds[i].fd, buf, sizeof(buf));
                if (n <= 0) {
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    --nfds;
                    --i;
                }
            }
        }
    }

    return 0;
}