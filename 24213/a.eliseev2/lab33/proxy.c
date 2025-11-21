#ifdef __linux__
#define _GNU_SOURCE // Needed for accept4
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CONN_QUEUE_SIZE SOMAXCONN
#define CONN_MAX 510
#define BUF_SIZE 1024

typedef enum {
    // Pending accept or decline from the server
    STATE_PENDING = 0,
    // Ready to transmit data
    STATE_OPEN = 1,
    // To be closed at the end of the event loop
    STATE_CLOSING = 2
} conn_state_t;

typedef struct {
    char data[BUF_SIZE]; // The actual buffer
    ssize_t recv_cnt;    // How many bytes are in the buffer
    ssize_t send_cnt;    // How many bytes have been sent
    int eof;             // Nonzero means we've reached EOF with recv
} buf_t;

typedef struct {
    int fd_in;          // Client socket
    int fd_out;         // Server socket
    conn_state_t state; // Connection state
    buf_t in_out_buf;   // Client -> server buffer
    buf_t out_in_buf;   // Server -> client buffer
} conn_t;

typedef struct {
    struct pollfd listen; // Socket listening for client connections
    struct pollfd_pair {
        struct pollfd in;  // Client socket
        struct pollfd out; // Server socket
    } conns[CONN_MAX];
} pollfds_t;

typedef struct {
    int fd_server;
    int conn_cnt;
    conn_t conns[CONN_MAX];
    pollfds_t pollfds;
} proxy_t;

int proxy_can_accept(proxy_t *proxy) {
    return proxy->conn_cnt < CONN_MAX;
}

nfds_t proxy_pollfd_count(proxy_t *proxy) {
    return 1 + proxy->conn_cnt * 2;
}

void proxy_add(proxy_t *proxy, int fd_in, int fd_out, conn_state_t state) {
    int conn_index = proxy->conn_cnt++;

    memset(&proxy->conns[conn_index], 0, sizeof(conn_t));
    proxy->conns[conn_index].fd_in = fd_in;
    proxy->conns[conn_index].fd_out = fd_out;
    proxy->conns[conn_index].state = state;

    proxy->pollfds.conns[conn_index].in.fd = fd_in;
    proxy->pollfds.conns[conn_index].out.fd = fd_out;
    switch (state) {
    case STATE_PENDING:
        proxy->pollfds.conns[conn_index].in.events = 0;
        proxy->pollfds.conns[conn_index].out.events = POLLOUT;
        break;
    case STATE_OPEN:
        proxy->pollfds.conns[conn_index].in.events = POLLRDNORM;
        proxy->pollfds.conns[conn_index].out.events = POLLRDNORM;
        break;
    default:
        break;
    }
}

int proxy_init(proxy_t *proxy, int fd_server) {
    proxy->fd_server = fd_server;
    proxy->conn_cnt = 0;

    proxy->pollfds.listen.fd = fd_server;
    proxy->pollfds.listen.events = POLLIN;
    return 0;
}

void proxy_destroy(proxy_t *proxy) {
    if (close(proxy->fd_server)) {
        perror("Close failed");
    }
    for (int i = 0; i < proxy->conn_cnt; i++) {
        if (close(proxy->conns[i].fd_in)) {
            perror("Close failed");
        }
        if (close(proxy->conns[i].fd_out)) {
            perror("Close failed");
        }
    }
}

typedef struct {
    char *remote_host;
    in_port_t remote_port;
    in_port_t local_port;
} args_t;

int parse_args(int argc, char **argv, args_t *args) {
    args->remote_host = NULL;
    args->remote_port = 0;
    args->local_port = 0;

    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <remote host> <remote port> "
                "<local port>\n",
                argv[0]);
        return 1;
    }
    args->remote_host = argv[1];
    const int port_min = 1;
    const int port_max = 65535;
    errno = 0;
    args->remote_port = atoi(argv[2]);
    if (errno || args->remote_port < port_min || args->remote_port > port_max) {
        fprintf(stderr, "Remote port must be a number between 1 and 65535.\n");
        return 1;
    }
    errno = 0;
    args->local_port = atoi(argv[3]);
    if (errno || args->local_port < port_min || args->local_port > port_max) {
        fprintf(stderr, "Local port must be a number between 1 and 65535.\n");
        return 1;
    }
    return 0;
}

int resolve_address(char *hostname, struct in_addr *address) {
    struct hostent *hosts = gethostbyname(hostname);
    if (!hosts) {
        fprintf(stderr, "Could not resolve remote host \"%s\".\n", hostname);
        return 1;
    }
    memcpy(address, *hosts->h_addr_list, sizeof(struct in_addr));
    return 0;
}

int setup_server_socket(in_port_t port, int *fd) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket for listening");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("Could not bind socket for listening");
        if (close(sock)) {
            perror("Close failed");
        }
        return 1;
    }

    if (listen(sock, CONN_QUEUE_SIZE)) {
        perror("Could not listen on socket");
        if (close(sock)) {
            perror("Close failed");
        }
        return 1;
    }

    *fd = sock;
    return 0;
}

int setup_client_socket(struct in_addr addr, in_port_t port, int *fd,
                        conn_state_t *state) {
    int sock = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock == -1) {
        perror("Could not create an output socket");
        return 1;
    }

    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr = addr;
    sock_addr.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr))) {
        if (errno == EINPROGRESS) {
            *state = STATE_PENDING;
            *fd = sock;
            return 0;
        }
        perror("Could not connect an output socket");
        if (close(sock)) {
            perror("Close failed");
        }
        return 1;
    }

    *state = STATE_OPEN;
    *fd = sock;
    return 0;
}

int accept_connection(proxy_t *proxy, int *fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int sock = accept4(proxy->fd_server, (struct sockaddr *)&addr, &addr_len,
                       SOCK_NONBLOCK);
    if (sock == -1) {
        perror("Could not accept connection");
        return 1;
    }
    char address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr.s_addr, address, sizeof(address));
    printf("Incoming connection from %s:%hu\n", address, ntohs(addr.sin_port));
    *fd = sock;
    return 0;
}

void handle_new_client(proxy_t *proxy, struct in_addr remote_addr,
                       in_port_t remote_port) {
    if (!(proxy->pollfds.listen.revents & POLLIN)) {
        return;
    }
    if (!proxy_can_accept(proxy)) {
        return;
    }
    int fd_in, fd_out;
    conn_state_t state;
    if (accept_connection(proxy, &fd_in)) {
        return;
    }
    if (setup_client_socket(remote_addr, remote_port, &fd_out, &state)) {
        if (close(fd_in)) {
            perror("Close failed");
        }
        return;
    }
    proxy_add(proxy, fd_in, fd_out, state);
    printf("Connected!\n");
}

int service_pending(conn_t *conn, struct pollfd_pair *pollfd) {
    if (!(pollfd->out.revents & POLLOUT)) {
        return 0;
    }
    int error;
    socklen_t error_size = sizeof(error);
    if (getsockopt(conn->fd_out, SOL_SOCKET, SO_ERROR, &error, &error_size)) {
        perror("Could not get connect result");
        return 1;
    }
    if (error) {
        return 1;
    }
    conn->state = STATE_OPEN;
    pollfd->in.events = POLLRDNORM;
    pollfd->out.events = POLLRDNORM;
    return 0;
}

int service_half_duplex(struct pollfd *from, struct pollfd *to, buf_t *buf) {
    if (from->revents & POLLRDNORM) {
        ssize_t recv_cnt = recv(from->fd, buf->data + buf->recv_cnt,
                                sizeof(buf->data) - buf->recv_cnt, 0);
        if (recv_cnt == -1) {
            perror("Recv failed");
            return 1;
        }
        if (recv_cnt == 0) {
            buf->eof = 1;
        }
        buf->recv_cnt += recv_cnt;
    }
    if (to->revents & POLLOUT) {
        ssize_t send_cnt = send(to->fd, buf->data + buf->send_cnt,
                                buf->recv_cnt - buf->send_cnt, MSG_NOSIGNAL);
        if (send_cnt == -1) {
            perror("Send failed");
            return 1;
        }
        buf->send_cnt += send_cnt;
        if (buf->send_cnt == buf->recv_cnt) {
            buf->send_cnt = 0;
            buf->recv_cnt = 0;
        }
    }
    if (buf->eof && buf->recv_cnt == 0) {
        // Nothing left to receive or send.
        // Peer terminated the connection, we need to close both sockets.
        return 1;
    }
    from->events &= ~POLLRDNORM;
    to->events &= ~POLLOUT;
    if (!buf->eof && buf->recv_cnt < sizeof(buf->data)) {
        from->events |= POLLRDNORM;
    }
    if (buf->send_cnt < buf->recv_cnt) {
        to->events |= POLLOUT;
    }
    return 0;
}

void service_connection(proxy_t *proxy, int index) {
    conn_t *conn = &proxy->conns[index];
    struct pollfd_pair *pollfd = &proxy->pollfds.conns[index];

    const int bad_events = POLLERR | POLLHUP;
    if (pollfd->in.revents & bad_events || pollfd->out.revents & bad_events) {
        conn->state = STATE_CLOSING;
        return;
    }

    int res = 0;
    switch (conn->state) {
    case STATE_PENDING:
        res = service_pending(conn, pollfd);
        break;
    case STATE_OPEN:
        res =
            service_half_duplex(&pollfd->in, &pollfd->out, &conn->in_out_buf) ||
            service_half_duplex(&pollfd->out, &pollfd->in, &conn->out_in_buf);
        break;
    default:
        break;
    }
    if (res) {
        conn->state = STATE_CLOSING;
    }
}

void close_marked(proxy_t *proxy) {
    for (int i = proxy->conn_cnt - 1; i >= 0; i--) {
        if (proxy->conns[i].state == STATE_CLOSING) {
            if (close(proxy->conns[i].fd_in)) {
                perror("Close failed");
            }
            if (close(proxy->conns[i].fd_out)) {
                perror("Close failed");
            }
            int elements_to_move = proxy->conn_cnt - i - 1;
            memmove(&proxy->conns[i], &proxy->conns[i + 1],
                    sizeof(conn_t) * elements_to_move);
            memmove(&proxy->pollfds.conns[i], &proxy->pollfds.conns[i + 1],
                    sizeof(struct pollfd_pair) * elements_to_move);
            proxy->conn_cnt--;
            printf("Disconnected!\n");
        }
    }
}

int main(int argc, char **argv) {
    args_t args;
    if (parse_args(argc, argv, &args)) {
        return 1;
    }
    struct in_addr remote_addr;
    if (resolve_address(args.remote_host, &remote_addr)) {
        return 1;
    }
    int server_fd;
    if (setup_server_socket(args.local_port, &server_fd)) {
        return 1;
    }

    static proxy_t proxy;
    if (proxy_init(&proxy, server_fd)) {
        proxy_destroy(&proxy);
        return 1;
    }

    while (1) {
        struct pollfd *pollfds = (struct pollfd *)&proxy.pollfds;
        int pollfd_count = proxy_pollfd_count(&proxy);
        if (poll(pollfds, pollfd_count, -1) == -1) {
            perror("Poll failed");
            break;
        }
        for (int i = 0; i < proxy.conn_cnt; i++) {
            service_connection(&proxy, i);
        }
        close_marked(&proxy);
        handle_new_client(&proxy, remote_addr, args.remote_port);
    }
    proxy_destroy(&proxy);
    return 1;
}
