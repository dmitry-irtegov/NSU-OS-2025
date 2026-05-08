#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#include <errno.h>

#define BUF_SIZE 8192
#define MAX_SESSIONS 510

volatile sig_atomic_t flag = 1;

typedef struct {
    char data[BUF_SIZE];
    int head;
    int tail;
} Buffer;

typedef struct {
    int active;
    int fd_client;
    int fd_server;
    Buffer client_to_server;
    Buffer server_to_client;
    int pfd_client_idx;
    int pfd_server_idx;
    int eof_client;
    int eof_server;
    int shut_wr_client;
    int shut_wr_server;
} Session;

struct addrinfo *target_addrinfo = NULL;
Session sessions[MAX_SESSIONS];

void sig_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        flag = 0;
    }
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void close_session(Session *s) {
    if (!s->active) {
        return;
    }

    shutdown(s->fd_client, SHUT_RDWR);
    close(s->fd_client);

    if (s->fd_server >= 0) {
        shutdown(s->fd_server, SHUT_RDWR);
        close(s->fd_server);
    }

    s->active = 0;
}

void shift_buffer(Buffer *b) {
    if (b->head == b->tail) {
        b->head = b->tail = 0;
    } else if (b->head > 0 && b->tail == BUF_SIZE) {
        int len = b->tail - b->head;
        memmove(b->data, b->data + b->head, len);
        b->head = 0;
        b->tail = len;
    }
}

void resolve_target(const char *target_host, const char *target_port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(target_host, target_port, &hints, &target_addrinfo) != 0) {
        perror("Ошибка getaddrinfo для узла N");
        exit(EXIT_FAILURE);
    }
}

int init_listen_socket(const char *port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Ошибка создания слушающего сокета");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(port));

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("Ошибка listen");
        exit(EXIT_FAILURE);
    }

    return listen_fd;
}

void init_sessions() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        sessions[i].active = 0;
    }
}

int build_poll_array(struct pollfd *pfds, int listen_fd) {
    int nfds = 0;
    int free_slots = 0;

    pfds[nfds].fd = listen_fd;
    pfds[nfds].events = 0;
    pfds[nfds].revents = 0;
    nfds++;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions[i].active) {
            free_slots++;
            continue;
        }

        shift_buffer(&sessions[i].client_to_server);
        shift_buffer(&sessions[i].server_to_client);

        int client_idx = nfds++;
        sessions[i].pfd_client_idx = client_idx;
        pfds[client_idx].fd = sessions[i].fd_client;
        pfds[client_idx].events = 0;
        pfds[client_idx].revents = 0;

        if (!sessions[i].eof_client && sessions[i].client_to_server.tail < BUF_SIZE) {
            pfds[client_idx].events |= POLLIN;
        }

        if (sessions[i].server_to_client.head < sessions[i].server_to_client.tail && !sessions[i].shut_wr_client) {
            pfds[client_idx].events |= POLLOUT;
        }

        int server_idx = nfds++;
        sessions[i].pfd_server_idx = server_idx;
        pfds[server_idx].fd = sessions[i].fd_server;
        pfds[server_idx].events = 0;
        pfds[server_idx].revents = 0;

        if (!sessions[i].eof_server && sessions[i].server_to_client.tail < BUF_SIZE) {
            pfds[server_idx].events |= POLLIN;
        }

        if (sessions[i].client_to_server.head < sessions[i].client_to_server.tail && !sessions[i].shut_wr_server) {
            pfds[server_idx].events |= POLLOUT;
        }
    }

    if (free_slots > 0) {
        pfds[0].events |= POLLIN;
    }

    return nfds;
}

void handle_new_connection(int listen_fd) {
    int new_client_fd = accept(listen_fd, NULL, NULL);
    if (new_client_fd < 0) {
        return;
    }

    int free_idx = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions[i].active) {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1) {
        close(new_client_fd);
        return;
    }

    int new_server_fd = socket(target_addrinfo->ai_family, target_addrinfo->ai_socktype, target_addrinfo->ai_protocol);
    if (new_server_fd < 0 || connect(new_server_fd, target_addrinfo->ai_addr, target_addrinfo->ai_addrlen) < 0) {
        close(new_client_fd);
        if (new_server_fd >= 0) {
            close(new_server_fd);
        }
        return;
    }

    set_nonblock(new_client_fd);
    set_nonblock(new_server_fd);

    sessions[free_idx].active = 1;
    sessions[free_idx].fd_client = new_client_fd;
    sessions[free_idx].fd_server = new_server_fd;
    sessions[free_idx].client_to_server.head = sessions[free_idx].client_to_server.tail = 0;
    sessions[free_idx].server_to_client.head = sessions[free_idx].server_to_client.tail = 0;
    sessions[free_idx].eof_client = 0;
    sessions[free_idx].eof_server = 0;
    sessions[free_idx].shut_wr_client = 0;
    sessions[free_idx].shut_wr_server = 0;
}

void handle_session_io(Session *s, struct pollfd *pfds) {
    int client_idx = s->pfd_client_idx;
    int server_idx = s->pfd_server_idx;
    int err = 0;

    if ((pfds[client_idx].revents & (POLLERR | POLLNVAL)) || 
        (pfds[server_idx].revents & (POLLERR | POLLNVAL))) {
        close_session(s);
        return;
    }

    if (!s->eof_client && (pfds[client_idx].revents & POLLIN)) {
        int num = recv(s->fd_client, s->client_to_server.data + s->client_to_server.tail, BUF_SIZE - s->client_to_server.tail, 0);
        if (num < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                perror("recv client error");
                err = 1;
            }
        } else if (num == 0) {
            s->eof_client = 1;
        } else {
            s->client_to_server.tail += num;
        }
    }

    if (!s->eof_server && !err && (pfds[server_idx].revents & POLLIN)) {
        int num = recv(s->fd_server, s->server_to_client.data + s->server_to_client.tail, BUF_SIZE - s->server_to_client.tail, 0);
        if (num < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                perror("recv server error");
                err = 1;
            }
        } else if (num == 0) {
            s->eof_server = 1;
        } else {
            s->server_to_client.tail += num;
        }
    }

    if (!err && (pfds[client_idx].revents & POLLOUT)) {
        int len = s->server_to_client.tail - s->server_to_client.head;
        if (len > 0) {
            int n = send(s->fd_client, s->server_to_client.data + s->server_to_client.head, len, 0);
            if (n < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                    perror("send client error");
                    err = 1;
                }
            } else {
                s->server_to_client.head += n;
            }
        }
    }

    if (!err && (pfds[server_idx].revents & POLLOUT)) {
        int len = s->client_to_server.tail - s->client_to_server.head;
        if (len > 0) {
            int n = send(s->fd_server, s->client_to_server.data + s->client_to_server.head, len, 0);
            if (n < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                    perror("send server error");
                    err = 1;
                }
            }
            else {
                s->client_to_server.head += n;
            }
        }
    }

    if (!err && s->eof_client && s->client_to_server.head == s->client_to_server.tail && !s->shut_wr_server) {
        shutdown(s->fd_server, SHUT_WR);
        s->shut_wr_server = 1;
    }

    if (!err && s->eof_server && s->server_to_client.head == s->server_to_client.tail && !s->shut_wr_client) {
        shutdown(s->fd_client, SHUT_WR);
        s->shut_wr_client = 1;
    }

    if (err || (s->shut_wr_client && s->shut_wr_server)) {
        close_session(s);
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Invalid input. Incorrect count of argument.\n");
        return EXIT_FAILURE;
    }

    const char *listen_port = argv[1];
    const char *target_host = argv[2];
    const char *target_port = argv[3];

    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    resolve_target(target_host, target_port);
    int listen_fd = init_listen_socket(listen_port);
    init_sessions();

    struct pollfd pfds[MAX_SESSIONS * 2 + 1];

    while(flag) {
        int nfds = build_poll_array(pfds, listen_fd);

        if (poll(pfds, nfds, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Ошибка poll");
            break;
        }

        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (sessions[i].active) {
                handle_session_io(&sessions[i], pfds);
            }
        }

        if (pfds[0].revents & POLLIN) {
            handle_new_connection(listen_fd);
        }
    }

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].active) {
            close_session(&sessions[i]);
        }
    }

    if (target_addrinfo) {
        freeaddrinfo(target_addrinfo);
    }
    close(listen_fd);

    return 0;
}   