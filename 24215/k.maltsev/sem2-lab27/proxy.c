#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONNECTIONS 510
#define BUFFER_SIZE 4096

#define SIDE_CLIENT 0
#define SIDE_REMOTE 1

typedef struct {
    char data[BUFFER_SIZE];
    size_t pos;
    size_t len;
} Buffer;

typedef struct {
    int active;
    int client_fd;
    int remote_fd;
    Buffer c2r;
    Buffer r2c;
} Connection;

static Connection connections[MAX_CONNECTIONS];
static int active_count = 0;

static void die(const char *message)
{
    perror(message);
    exit(1);
}

static int buffer_empty(const Buffer *buffer)
{
    return buffer->pos == buffer->len;
}

static void buffer_clear(Buffer *buffer)
{
    buffer->pos = 0;
    buffer->len = 0;
}

static void close_connection(Connection *conn)
{
    if (!conn->active) {
        return;
    }

    if (conn->client_fd >= 0) {
        close(conn->client_fd);
    }

    if (conn->remote_fd >= 0) {
        close(conn->remote_fd);
    }

    conn->active = 0;
    conn->client_fd = -1;
    conn->remote_fd = -1;
    buffer_clear(&conn->c2r);
    buffer_clear(&conn->r2c);

    if (active_count > 0) {
        --active_count;
    }
}

static int create_listener(const char *port)
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int listen_fd;
    int yes = 1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    rc = getaddrinfo(NULL, port, &hints, &result);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo listen: %s\n", gai_strerror(rc));
        exit(1);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1) {
            continue;
        }

        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(listen_fd, SOMAXCONN) == 0) {
                freeaddrinfo(result);
                return listen_fd;
            }
        }

        close(listen_fd);
    }

    freeaddrinfo(result);
    fprintf(stderr, "cannot bind to port %s\n", port);
    exit(1);
}

static struct addrinfo *resolve_target(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *result;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    rc = getaddrinfo(host, port, &hints, &result);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo target: %s\n", gai_strerror(rc));
        exit(1);
    }

    return result;
}

static int connect_to_target(struct addrinfo *target)
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

static Connection *new_connection(int client_fd, int remote_fd)
{
    int i;

    for (i = 0; i < MAX_CONNECTIONS; ++i) {
        if (!connections[i].active) {
            connections[i].active = 1;
            connections[i].client_fd = client_fd;
            connections[i].remote_fd = remote_fd;
            buffer_clear(&connections[i].c2r);
            buffer_clear(&connections[i].r2c);
            ++active_count;
            return &connections[i];
        }
    }

    return NULL;
}

static void accept_client(int listen_fd, struct addrinfo *target)
{
    int client_fd;
    int remote_fd;

    client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd == -1) {
        if (errno == EINTR) {
            return;
        }
        perror("accept");
        return;
    }

    if (active_count >= MAX_CONNECTIONS) {
        close(client_fd);
        return;
    }

    remote_fd = connect_to_target(target);
    if (remote_fd == -1) {
        close(client_fd);
        return;
    }

    if (new_connection(client_fd, remote_fd) == NULL) {
        close(client_fd);
        close(remote_fd);
    }
}

static int read_to_buffer(int fd, Buffer *buffer)
{
    ssize_t n;

    n = read(fd, buffer->data, sizeof(buffer->data));
    if (n > 0) {
        buffer->pos = 0;
        buffer->len = (size_t)n;
        return 0;
    }

    if (n == 0) {
        return -1;
    }

    if (errno == EINTR || errno == EAGAIN) {
        return 0;
    }

    return -1;
}

static int write_from_buffer(int fd, Buffer *buffer)
{
    ssize_t n;
    size_t count;

    count = buffer->len - buffer->pos;

    n = write(fd, buffer->data + buffer->pos, count);
    if (n > 0) {
        buffer->pos += (size_t)n;

        if (buffer->pos == buffer->len) {
            buffer_clear(buffer);
        }

        return 0;
    }

    if (n == 0) {
        return -1;
    }

    if (errno == EINTR || errno == EAGAIN) {
        return 0;
    }

    return -1;
}

static void handle_event(Connection *conn, int side, short revents)
{
    if (!conn->active) {
        return;
    }

    if (revents & (POLLERR | POLLNVAL | POLLHUP)) {
        close_connection(conn);
        return;
    }

    if (side == SIDE_CLIENT) {
        if ((revents & POLLOUT) && !buffer_empty(&conn->r2c)) {
            if (write_from_buffer(conn->client_fd, &conn->r2c) == -1) {
                close_connection(conn);
                return;
            }
        }

        if ((revents & POLLIN) && buffer_empty(&conn->c2r)) {
            if (read_to_buffer(conn->client_fd, &conn->c2r) == -1) {
                close_connection(conn);
                return;
            }
        }
    } else {
        if ((revents & POLLOUT) && !buffer_empty(&conn->c2r)) {
            if (write_from_buffer(conn->remote_fd, &conn->c2r) == -1) {
                close_connection(conn);
                return;
            }
        }

        if ((revents & POLLIN) && buffer_empty(&conn->r2c)) {
            if (read_to_buffer(conn->remote_fd, &conn->r2c) == -1) {
                close_connection(conn);
                return;
            }
        }
    }
}

int main(int argc, char **argv)
{
    const char *listen_port;
    const char *target_host;
    const char *target_port;

    int listen_fd;
    struct addrinfo *target;

    struct pollfd fds[1 + MAX_CONNECTIONS * 2];
    int conn_index[1 + MAX_CONNECTIONS * 2];
    int side_index[1 + MAX_CONNECTIONS * 2];

    nfds_t nfds;
    int i;
    int rc;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <listen_port> <target_host> <target_port>\n", argv[0]);
        return 1;
    }

    listen_port = argv[1];
    target_host = argv[2];
    target_port = argv[3];

    signal(SIGPIPE, SIG_IGN);

    listen_fd = create_listener(listen_port);
    target = resolve_target(target_host, target_port);

    fprintf(stderr, "proxy: listening on port %s, forwarding to %s:%s\n",
            listen_port, target_host, target_port);

    for (;;) {
        nfds = 0;

        fds[nfds].fd = listen_fd;
        fds[nfds].events = POLLIN;
        fds[nfds].revents = 0;
        conn_index[nfds] = -1;
        side_index[nfds] = -1;
        ++nfds;

        for (i = 0; i < MAX_CONNECTIONS; ++i) {
            if (!connections[i].active) {
                continue;
            }

            fds[nfds].fd = connections[i].client_fd;
            fds[nfds].events = 0;
            fds[nfds].revents = 0;

            if (buffer_empty(&connections[i].c2r)) {
                fds[nfds].events |= POLLIN;
            }

            if (!buffer_empty(&connections[i].r2c)) {
                fds[nfds].events |= POLLOUT;
            }

            conn_index[nfds] = i;
            side_index[nfds] = SIDE_CLIENT;
            ++nfds;

            fds[nfds].fd = connections[i].remote_fd;
            fds[nfds].events = 0;
            fds[nfds].revents = 0;

            if (buffer_empty(&connections[i].r2c)) {
                fds[nfds].events |= POLLIN;
            }

            if (!buffer_empty(&connections[i].c2r)) {
                fds[nfds].events |= POLLOUT;
            }

            conn_index[nfds] = i;
            side_index[nfds] = SIDE_REMOTE;
            ++nfds;
        }

        rc = poll(fds, nfds, -1);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            die("poll");
        }

        if (fds[0].revents & POLLIN) {
            accept_client(listen_fd, target);
        }

        for (i = 1; i < (int)nfds; ++i) {
            if (fds[i].revents == 0) {
                continue;
            }

            handle_event(&connections[conn_index[i]], side_index[i], fds[i].revents);
        }
    }

    freeaddrinfo(target);
    close(listen_fd);

    return 0;
}