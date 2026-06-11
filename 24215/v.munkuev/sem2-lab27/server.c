#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CONN 510
#define BUF_SIZE 8192
#define WRITE_CHUNK 4096
#define MAX_POLL_FDS (1 + MAX_CONN * 2)

typedef struct Buffer {
    char data[BUF_SIZE];
    size_t len;
} Buffer;

typedef struct Connection {
    int active;
    int client_fd;
    int server_fd;

    Buffer ctos; //от клиента к удалённому серверу
    Buffer stoc; //от удалённого сервера к клиенту
} Connection;

typedef enum PollKind {
    POLL_LISTENER,
    POLL_CLIENT,
    POLL_SERVER
} PollKind;

typedef struct PollRef {
    PollKind kind;
    int connection_index;
} PollRef;

static Connection conns[MAX_CONN];
static int active_count = 0;
static struct addrinfo *remote_addrinfo = NULL;

void handle_error(int error_number) {
    gai_strerror(error_number);
}

static void buffer_consume(Buffer *buf, size_t n) {
    if(n >= buf->len) {
        buf->len = 0;
        return;
    }

    memmove(buf->data, buf->data + n, buf->len - n);
    buf->len -= n;
}

static size_t buffer_free(const Buffer *buf) {
    return BUF_SIZE - buf->len;
}

static void close_connection(int idx) {
    Connection *c = &conns[idx];

    if(!c->active) {
        return;
    }

    if(c->client_fd >= 0) {
        close(c->client_fd);
    }

    if(c->server_fd >= 0) {
        close(c->server_fd);
    }

    c->active = 0;
    c->client_fd = -1;
    c->server_fd = -1;
    c->ctos.len = 0;
    c->stoc.len = 0;

    active_count--;
}

static int find_free_slot(void) {
    for(int i = 0; i < MAX_CONN; i++) {
        if(!conns[i].active) {
            return i;
        }
    }

    return -1;
}

static int make_listener(const char *port) {
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp = NULL;
    int fd = -1;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, port, &hints, &res);
    if(err != 0) {
        perror("getaddrinfo");
        return 1;
    }

    for(rp = res; rp != NULL; rp = rp->ai_next) {
        int opt = 1;

        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(fd < 0) {
            continue;
        }

        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            close(fd);
            fd = -1;
            continue;
        }

        if(bind(fd, rp->ai_addr, rp->ai_addrlen) < 0) {
            close(fd);
            fd = -1;
            continue;
        }

        if(listen(fd, SOMAXCONN) < 0) {
            close(fd);
            fd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if(fd < 0) {
        fprintf(stderr, "cannot bind/listen on port %s\n", port);
        exit(1);
    }

    return fd;
}

static void resolve_remote(const char *host, const char *port) {
    struct addrinfo hints;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(host, port, &hints, &remote_addrinfo);
    if(err != 0) {
        perror("getaddrinfo in resolve_remote");
        handle_error(err);curl
    }
}

static int connect_remote_server(void) {
    struct addrinfo *rp;

    for(rp = remote_addrinfo; rp != NULL; rp = rp->ai_next) {
        int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(fd < 0) {
            continue;
        }

        if(connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            return fd;
        }

        close(fd);
    }

    return -1;
}

static void accept_client(int listener_fd) {
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd;
    int server_fd;
    int slot;

    if(active_count >= MAX_CONN) {
        return;
    }

    slot = find_free_slot();
    if(slot < 0) {
        return;
    }

    client_fd = accept(listener_fd, &addr, &addrlen);
    if(client_fd < 0) {
        if(errno == EINTR) {
            return;
        }

        perror("accept");
        return;
    }

    server_fd = connect_remote_server();
    if(server_fd < 0) {
        close(client_fd);
        return;
    }

    conns[slot].active = 1;
    conns[slot].client_fd = client_fd;
    conns[slot].server_fd = server_fd;
    conns[slot].ctos.len = 0;
    conns[slot].stoc.len = 0;

    active_count++;
}

static void add_pollfd(struct pollfd *poll_fds, PollRef *refs, nfds_t *nfds, int fd, short events, PollKind kind, int connection_index) {
    poll_fds[*nfds].fd = fd;
    poll_fds[*nfds].events = events;
    poll_fds[*nfds].revents = 0;

    refs[*nfds].kind = kind;
    refs[*nfds].connection_index = connection_index;

    (*nfds)++;
}

static int read_to_buffer(int idx, int fd, Buffer *buf) {
    ssize_t n;
    size_t free_space = buffer_free(buf);

    if(free_space == 0) {
        return 0;
    }

    n = read(fd, buf->data + buf->len, free_space);

    if(n > 0) {
        buf->len += n;
        return 0;
    }

    if(n == 0) {
        close_connection(idx);
        return -1;
    }

    if(errno == EINTR) {
        return 0;
    }

    close_connection(idx);
    return -1;
}

static int write_from_buffer(int idx, int fd, Buffer *buf) {
    ssize_t n;
    size_t to_write;

    if(buf->len == 0) {
        return 0;
    }

    to_write = buf->len;
    if(to_write > WRITE_CHUNK) {
        to_write = WRITE_CHUNK;
    }

    n = write(fd, buf->data, to_write);

    if(n > 0) {
        buffer_consume(buf, (size_t)n);
        return 0;
    }

    if(n == 0) {
        return 0;
    }

    if(errno == EINTR) {
        return 0;
    }

    close_connection(idx);
    return -1;
}

static void handle_client_event(int idx, short events) {
    Connection *c = &conns[idx];

    if(!c->active) {
        return;
    }

    if(events & (POLLERR | POLLNVAL)) {
        close_connection(idx);
        return;
    }

    //client_fd:
    // - читаем из клиента в буфер ctos;
    // - пишем клиенту из буфера stoc.

    if(events & POLLOUT) {
        if(write_from_buffer(idx, c->client_fd, &c->stoc) < 0) {
            return;
        }
    }

    if(events & POLLIN) {
        if(read_to_buffer(idx, c->client_fd, &c->ctos) < 0) {
            return;
        }
    }

    if((events & POLLHUP) && !(events & POLLIN)) {
        close_connection(idx);
    }
}

static void handle_server_event(int idx, short revents) {
    Connection *c = &conns[idx];

    if(!c->active) {
        return;
    }

    if(revents & (POLLERR | POLLNVAL)) {
        close_connection(idx);
        return;
    }

    //server_fd:
    //читаем из удалённого сервера в буфер stoc;
    //пишем удалённому серверу из буфера ctos.

    if(revents & POLLOUT) {
        if(write_from_buffer(idx, c->server_fd, &c->ctos) < 0) {
            return;
        }
    }

    if(revents & POLLIN) {
        if(read_to_buffer(idx, c->server_fd, &c->stoc) < 0) {
            return;
        }
    }

    if((revents & POLLHUP) && !(revents & POLLIN)) {
        close_connection(idx);
    }
}

static void init_connections(void) {
    for(int i = 0; i < MAX_CONN; i++) {
        conns[i].active = 0;
        conns[i].client_fd = -1;
        conns[i].server_fd = -1;
        conns[i].ctos.len = 0;
        conns[i].stoc.len = 0;
    }
}

static void event_loop(int listener_fd) {
    struct pollfd poll_fds[MAX_POLL_FDS];
    PollRef refs[MAX_POLL_FDS];

    for(;;) {
        nfds_t nfds = 0;
        int ready;

        if(active_count < MAX_CONN) {
            add_pollfd(poll_fds, refs, &nfds, listener_fd, POLLIN, POLL_LISTENER, -1);
        }

        for(int i = 0; i < MAX_CONN; i++) {
            Connection *c = &conns[i];
            short client_events = 0;
            short server_events = 0;

            if(!c->active) {
                continue;
            }

            if(buffer_free(&c->ctos) > 0) {
                client_events |= POLLIN;
            }

            if(c->stoc.len > 0) {
                client_events |= POLLOUT;
            }

            if(buffer_free(&c->stoc) > 0) {
                server_events |= POLLIN;
            }

            if(c->ctos.len > 0) {
                server_events |= POLLOUT;
            }

            add_pollfd(poll_fds, refs, &nfds, c->client_fd, client_events, POLL_CLIENT, i);

            add_pollfd(poll_fds, refs, &nfds, c->server_fd, server_events, POLL_SERVER, i);
        }

        ready = poll(poll_fds, nfds, -1);
        if(ready < 0) {
            if(errno == EINTR) {
                continue;
            }

            perror("poll");
            exit(1);
        }

        for(nfds_t i = 0; i < nfds; i++) {
            short revents = poll_fds[i].revents;

            if(revents == 0) {
                continue;
            }

            if(refs[i].kind == POLL_LISTENER) {
                accept_client(listener_fd);
            }
            else if(refs[i].kind == POLL_CLIENT) {
                handle_client_event(refs[i].connection_index, revents);
            }
            else if(refs[i].kind == POLL_SERVER) {
                handle_server_event(refs[i].connection_index, revents);
            }
        }
    }
}

int main(int argc, char **argv) {
    int listener_fd;

    if(argc != 4) {
        fprintf(stderr, "usage: %s <listen-port> <remote-host> <remote-port>\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    init_connections();

    resolve_remote(argv[2], argv[3]);
    listener_fd = make_listener(argv[1]);

    event_loop(listener_fd);

    close(listener_fd);
    freeaddrinfo(remote_addrinfo);

    return 0;
}