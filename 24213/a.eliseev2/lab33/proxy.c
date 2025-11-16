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

#define PORT_MIN 1
#define PORT_MAX 65535

#define BUF_SIZE 2048

#define CONNCNT_MAX 510
#define POLLFD_CNT 1021 // 510 * 2 + 1

typedef struct {
    int fd_in;
    int fd_out;
    char close;
} conn_t;

typedef struct {
    int fd_server;
    conn_t conns[CONNCNT_MAX];
    int conn_cnt;
    struct pollfd pollfds[POLLFD_CNT];
} proxy_t;

proxy_t proxy;

void proxy_init(proxy_t *proxy, int fd_server) {
    proxy->fd_server = fd_server;
    proxy->pollfds[0].fd = fd_server;
    proxy->pollfds[0].events = POLLIN;
    proxy->pollfds[0].revents = 0;
    proxy->conn_cnt = 0;
}

int proxy_can_accept(proxy_t *proxy) {
    return proxy->conn_cnt < CONNCNT_MAX;
}

int proxy_pollcnt(proxy_t *proxy) {
    return proxy->conn_cnt * 2 + 1;
}

void proxy_add(proxy_t *proxy, int fd_in, int fd_out) {
    int conn_index = proxy->conn_cnt++;
    proxy->conns[conn_index] =
        (conn_t){.fd_in = fd_in, .fd_out = fd_out, .close = 0};
    struct pollfd *pollfd_in = &proxy->pollfds[conn_index * 2 + 1];
    struct pollfd *pollfd_out = &proxy->pollfds[conn_index * 2 + 2];
    pollfd_in->fd = fd_in;
    pollfd_in->events = POLLRDNORM;
    pollfd_out->fd = fd_out;
    pollfd_out->events = POLLRDNORM;
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
    errno = 0;
    args->remote_port = atoi(argv[2]);
    if (errno || args->remote_port < PORT_MIN || args->remote_port > PORT_MAX) {
        fprintf(stderr, "Remote port must be a number between 1 and 65535.\n");
        return 1;
    }
    errno = 0;
    args->local_port = atoi(argv[3]);
    if (errno || args->local_port < PORT_MIN || args->local_port > PORT_MAX) {
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

    int opt_val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val))) {
        perror("Could not set REUSEADDR on socket");
        if (close(sock)) {
            perror("Close failed");
        }
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

    if (listen(sock, SOMAXCONN)) {
        perror("Could not listen on socket");
        if (close(sock)) {
            perror("Close failed");
        }
        return 1;
    }

    *fd = sock;
    return 0;
}

int setup_client_socket(struct in_addr addr, in_port_t port, int *fd) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create an output socket");
        return 1;
    }

    int opt_val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val))) {
        perror("Could not set REUSEADDR on socket");
        if (close(sock)) {
            perror("Close failed");
        }
        return 1;
    }

    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_in));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr = addr;
    sock_addr.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr))) {
        perror("Could not connect an output socket");
        if (close(sock)) {
            perror("Close failed");
        }
        return 1;
    }

    *fd = sock;
    return 0;
}

int accept_connection(proxy_t *proxy, int *fd) {
    int sock = accept(proxy->fd_server, NULL, NULL);
    if (sock == -1) {
        perror("Could not accept connection");
        return 1;
    }
    *fd = sock;
    return 0;
}

void handle_new_client(proxy_t *proxy, struct in_addr remote_addr,
                       in_port_t remote_port) {
    if (!proxy_can_accept(proxy)) {
        return;
    }
    int fd_in, fd_out;
    if (accept_connection(proxy, &fd_in)) {
        return;
    }
    if (setup_client_socket(remote_addr, remote_port, &fd_out)) {
        if (close(fd_in)) {
            perror("Close failed");
        }
        return;
    }
    proxy_add(proxy, fd_in, fd_out);
}

int forward_data(int fd_from, int fd_to) {
    char buffer[BUF_SIZE];

    ssize_t recv_size =
        recv(fd_from, buffer, sizeof(buffer), MSG_DONTWAIT | MSG_PEEK);
    if (recv_size == 0) {
        return 1;
    }
    if (recv_size == -1) {
        perror("Recv failed");
        return 1;
    }
    ssize_t send_size = send(fd_to, buffer, recv_size, MSG_NOSIGNAL);
    if (send_size == -1) {
        perror("Send failed");
        return 1;
    }
    ssize_t remove_size = recv(fd_from, buffer, send_size, MSG_DONTWAIT);
    if (remove_size == -1) {
        perror("Recv queue remove failed");
        return 1;
    }
    if (remove_size != send_size) {
        fprintf(stderr, "Recv queue remove size mismatch.\n");
        return 1;
    }
    return 0;
}

void service_connection(proxy_t *proxy, int index) {
    conn_t *conn = &proxy->conns[index];
    struct pollfd *pin = &proxy->pollfds[index * 2 + 1];
    struct pollfd *pout = &proxy->pollfds[index * 2 + 2];

    const int bad_events = POLLERR | POLLHUP;
    if (pin->revents & bad_events || pout->revents & bad_events) {
        conn->close = 1;
        return;
    }

    if (pin->revents & POLLRDNORM) {
        pout->events = POLLRDNORM | POLLOUT;
        if (pout->revents & POLLOUT && forward_data(pin->fd, pout->fd)) {
            conn->close = 1;
            return;
        }
    } else {
        pout->events = POLLRDNORM;
    }

    if (pout->revents & POLLRDNORM) {
        pin->events = POLLRDNORM | POLLOUT;
        if (pin->revents & POLLOUT && forward_data(pout->fd, pin->fd)) {
            conn->close = 1;
            return;
        }
    } else {
        pin->events = POLLRDNORM;
    }
}

void close_all(proxy_t *proxy) {
    for (int i = 0; i < proxy->conn_cnt; i++) {
        if (close(proxy->conns[i].fd_in)) {
            perror("Close failed");
        }
        if (close(proxy->conns[i].fd_out)) {
            perror("Close failed");
        }
    }
}

void close_marked(proxy_t *proxy) {
    for (int i = proxy->conn_cnt - 1; i >= 0; i--) {
        if (proxy->conns[i].close) {
            if (close(proxy->conns[i].fd_in)) {
                perror("Close failed");
            }
            if (close(proxy->conns[i].fd_out)) {
                perror("Close failed");
            }
            int elements_to_move = proxy->conn_cnt - i - 1;
            memmove(&proxy->conns[i], &proxy->conns[i + 1],
                    sizeof(conn_t) * elements_to_move);
            int i_pollfd = i * 2 + 1;
            memmove(&proxy->pollfds[i_pollfd], &proxy->pollfds[i_pollfd + 2],
                    2 * sizeof(struct pollfd) * elements_to_move);
            proxy->conn_cnt--;
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

    proxy_init(&proxy, server_fd);

    while (1) {
        if (poll(proxy.pollfds, proxy_pollcnt(&proxy), -1) == -1) {
            perror("Poll failed");
            break;
        }
        for (int i = 0; i < proxy.conn_cnt; i++) {
            service_connection(&proxy, i);
        }
        close_marked(&proxy);
        if (proxy.pollfds[0].revents & POLLIN) {
            handle_new_client(&proxy, remote_addr, args.remote_port);
        }
    }
    close_all(&proxy);
    if (close(server_fd)) {
        perror("Close failed");
    }
    return 1;
}
