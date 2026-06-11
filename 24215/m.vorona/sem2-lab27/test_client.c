#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define DEFAULT_CONN 510
#define MAX_CONN 900
#define MSG_SIZE 128
#define MAX_PFDS MAX_CONN

typedef struct {
    int fd;
    int done;
    char msg[MSG_SIZE];
    size_t msg_len;
    char recv_buf[MSG_SIZE];
    size_t recv_len;
} client_conn_t;

static client_conn_t conns[MAX_CONN];

static int connect_to_host(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *rp;
    int fd;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo(host, port, &hints, &res);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            freeaddrinfo(res);
            return fd;
        }

        close(fd);
    }

    freeaddrinfo(res);
    return -1;
}

static int write_all(int fd, const char *buf, size_t len) {
    size_t off = 0;

    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (n == 0) {
            return -1;
        }

        off += (size_t)n;
    }

    return 0;
}

int main(int argc, char **argv) {
    const char *host;
    const char *port;
    int count = DEFAULT_CONN;
    int done_count = 0;

    struct pollfd pfds[MAX_PFDS];
    int map[MAX_PFDS];

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <host> <port> [connections]\n", argv[0]);
        return 1;
    }

    host = argv[1];
    port = argv[2];

    if (argc == 4) {
        count = atoi(argv[3]);
    }

    if (count <= 0 || count > MAX_CONN) {
        fprintf(stderr, "Invalid connection count\n");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        conns[i].fd = connect_to_host(host, port);
        if (conns[i].fd == -1) {
            fprintf(stderr, "connect failed at %d\n", i);
            return 1;
        }

        conns[i].done = 0;
        conns[i].recv_len = 0;

        snprintf(conns[i].msg, sizeof(conns[i].msg), "message from connection %d\n", i);
        conns[i].msg_len = strlen(conns[i].msg);
    }

    printf("created %d connections\n", count);
    fflush(stdout);

    for (int i = 0; i < count; i++) {
        if (write_all(conns[i].fd, conns[i].msg, conns[i].msg_len) != 0) {
            fprintf(stderr, "write failed at %d\n", i);
            return 1;
        }
    }

    while (done_count < count) {
        int nfds = 0;

        for (int i = 0; i < count; i++) {
            if (conns[i].done) {
                continue;
            }

            pfds[nfds].fd = conns[i].fd;
            pfds[nfds].events = POLLIN;
            pfds[nfds].revents = 0;
            map[nfds] = i;
            nfds++;
        }

        if (poll(pfds, nfds, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("poll");
            return 1;
        }

        for (int p = 0; p < nfds; p++) {
            int i = map[p];

            if (pfds[p].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                fprintf(stderr, "connection %d closed with error\n", i);
                return 1;
            }

            if (pfds[p].revents & POLLIN) {
                ssize_t n = read(conns[i].fd,
                                 conns[i].recv_buf + conns[i].recv_len,
                                 sizeof(conns[i].recv_buf) - conns[i].recv_len);

                if (n <= 0) {
                    fprintf(stderr, "connection %d closed early\n", i);
                    return 1;
                }

                conns[i].recv_len += (size_t)n;

                if (conns[i].recv_len >= conns[i].msg_len) {
                    if (memcmp(conns[i].recv_buf, conns[i].msg, conns[i].msg_len) != 0) {
                        fprintf(stderr, "bad echo on connection %d\n", i);
                        return 1;
                    }

                    conns[i].done = 1;
                    done_count++;
                    close(conns[i].fd);
                }
            }
        }
    }

    printf("all %d connections passed\n", count);

    return 0;
}