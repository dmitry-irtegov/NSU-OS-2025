#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MESSAGE "test message from client\n"

static volatile sig_atomic_t running = 1;

static void stop_handler(int signo)
{
    (void)signo;
    running = 0;
}

static int connect_to_server(struct addrinfo *target)
{
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

static struct addrinfo *resolve_target(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        exit(1);
    }

    return res;
}

int main(int argc, char **argv)
{
    const char *host;
    const char *port;
    const char *message;
    int count;
    int *fds;
    int i;
    int connected;
    struct addrinfo *target;

    if (argc < 4 || argc > 5) {
        fprintf(stderr, "usage: %s <host> <port> <connection_count> [message]\n", argv[0]);
        return 1;
    }

    host = argv[1];
    port = argv[2];
    count = atoi(argv[3]);
    message = argc == 5 ? argv[4] : DEFAULT_MESSAGE;

    if (count <= 0) {
        fprintf(stderr, "connection_count must be positive\n");
        return 1;
    }

    signal(SIGINT, stop_handler);
    signal(SIGTERM, stop_handler);

    fds = calloc((size_t)count, sizeof(*fds));
    if (fds == NULL) {
        perror("calloc");
        return 1;
    }

    for (i = 0; i < count; ++i) {
        fds[i] = -1;
    }

    target = resolve_target(host, port);

    connected = 0;

    for (i = 0; i < count; ++i) {
        fds[i] = connect_to_server(target);

        if (fds[i] == -1) {
            fprintf(stderr, "connection %d failed\n", i + 1);
            break;
        }

        ++connected;

        if (write(fds[i], message, strlen(message)) == -1) {
            perror("write");
        }

        if (connected % 50 == 0 || connected == count) {
            printf("connected: %d\n", connected);
            fflush(stdout);
        }
    }

    printf("total connected: %d\n", connected);
    printf("press Ctrl+C to close connections\n");

    while (running) {
        sleep(1);
    }

    for (i = 0; i < connected; ++i) {
        if (fds[i] != -1) {
            close(fds[i]);
        }
    }

    freeaddrinfo(target);
    free(fds);

    printf("closed connections\n");

    return 0;
}