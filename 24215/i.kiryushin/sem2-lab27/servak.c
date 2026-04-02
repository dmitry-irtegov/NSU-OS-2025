#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

#define MAXLINE 1200
#define MAX_FDS 1024
#define MAX_CONNECTIONS 510

struct pollfd fds[MAX_FDS];
volatile sig_atomic_t interrupt_flag = 0;

typedef struct session {
    int sock_in;
    int sock_out;
    int fd_in_index;
    int fd_out_index;
    char buf_in[MAXLINE];
    int buf_in_size;
    int buf_in_sent;
    char buf_out[MAXLINE];
    int buf_out_size;
    int buf_out_sent;
} session;

void signal_handler(int signum) {
    (void)signum;
    interrupt_flag = 1;
}

void clear_fd_slot(int index) {
    if (index < 0 || index >= MAX_FDS) return;
    fds[index].fd = -1;
    fds[index].events = 0;
    fds[index].revents = 0;
}

void normalize_buffer(char *buf, int *size, int *sent) {
    if (*sent == 0) return;

    if (*sent >= *size) {
        *size = 0;
        *sent = 0;
        return;
    }

    memmove(buf, buf + *sent, *size - *sent);
    *size -= *sent;
    *sent = 0;
}

void update_events(session *s) {
    if (s->sock_in == -1 || s->sock_out == -1) return;

    normalize_buffer(s->buf_in, &s->buf_in_size, &s->buf_in_sent);
    normalize_buffer(s->buf_out, &s->buf_out_size, &s->buf_out_sent);

    fds[s->fd_in_index].events = 0;
    fds[s->fd_out_index].events = 0;

    if (s->buf_in_size < MAXLINE) {
        fds[s->fd_in_index].events |= POLLIN;
    }
    if (s->buf_out_size > 0) {
        fds[s->fd_in_index].events |= POLLOUT;
    }

    if (s->buf_out_size < MAXLINE) {
        fds[s->fd_out_index].events |= POLLIN;
    }
    if (s->buf_in_size > 0) {
        fds[s->fd_out_index].events |= POLLOUT;
    }
}

void clear_connection(session *sessions, int session_id) {
    if (sessions[session_id].sock_in != -1) close(sessions[session_id].sock_in);
    if (sessions[session_id].sock_out != -1) close(sessions[session_id].sock_out);

    clear_fd_slot(sessions[session_id].fd_in_index);
    clear_fd_slot(sessions[session_id].fd_out_index);

    sessions[session_id].sock_in = -1;
    sessions[session_id].sock_out = -1;
    sessions[session_id].fd_in_index = -1;
    sessions[session_id].fd_out_index = -1;
    sessions[session_id].buf_in_size = 0;
    sessions[session_id].buf_in_sent = 0;
    sessions[session_id].buf_out_size = 0;
    sessions[session_id].buf_out_sent = 0;
}

int get_listen_fd(int port) {
    int sockfd;
    int opt = 1;
    struct sockaddr_in addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        close(sockfd);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind error");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, MAX_CONNECTIONS) < 0) {
        perror("listen error");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int get_sending_fd(char *address, int port) {
    int sockfd;
    struct sockaddr_in addr;
    struct hostent *host;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
        host = gethostbyname(address);
        if (host == NULL) {
            close(sockfd);
            return -1;
        }
        memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int find_free_session(session *sessions) {
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (sessions[i].sock_in == -1) {
            return i;
        }
    }

    return -1;
}

int find_free_fd_slot() {
    int i;

    for (i = 1; i < MAX_FDS; i++) {
        if (fds[i].fd == -1) {
            return i;
        }
    }

    return -1;
}

int find_session_by_fd(session *sessions, int fd) {
    int i;

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (sessions[i].sock_in == fd || sessions[i].sock_out == fd) {
            return i;
        }
    }

    return -1;
}

int create_connection(session *sessions, char *address_out, int port_out, int listen_sock) {
    int session_id;
    int client_fd;
    int out_fd;
    int fd_in_index;
    int fd_out_index;

    client_fd = accept(listen_sock, NULL, NULL);
    if (client_fd < 0) {
        perror("accept error");
        return -1;
    }

    session_id = find_free_session(sessions);
    if (session_id == -1) {
        close(client_fd);
        return -1;
    }

    fd_in_index = find_free_fd_slot();
    if (fd_in_index == -1) {
        close(client_fd);
        return -1;
    }

    fds[fd_in_index].fd = -2;
    fd_out_index = find_free_fd_slot();
    fds[fd_in_index].fd = -1;
    if (fd_out_index == -1) {
        close(client_fd);
        return -1;
    }

    out_fd = get_sending_fd(address_out, port_out);
    if (out_fd < 0) {
        close(client_fd);
        return -1;
    }

    sessions[session_id].sock_in = client_fd;
    sessions[session_id].sock_out = out_fd;
    sessions[session_id].fd_in_index = fd_in_index;
    sessions[session_id].fd_out_index = fd_out_index;
    sessions[session_id].buf_in_size = 0;
    sessions[session_id].buf_in_sent = 0;
    sessions[session_id].buf_out_size = 0;
    sessions[session_id].buf_out_sent = 0;

    fds[fd_in_index].fd = client_fd;
    fds[fd_out_index].fd = out_fd;

    update_events(&sessions[session_id]);

    printf("New connection\n");
    return 0;
}

int event_data(session *sessions, int index) {
    int current_fd = fds[index].fd;
    int session_id = find_session_by_fd(sessions, current_fd);
    char *buf;
    int *buf_size;
    int *buf_sent;
    ssize_t bytes_read;

    if (session_id == -1) return -1;

    if (sessions[session_id].sock_in == current_fd) {
        buf = sessions[session_id].buf_in;
        buf_size = &sessions[session_id].buf_in_size;
        buf_sent = &sessions[session_id].buf_in_sent;
    } else {
        buf = sessions[session_id].buf_out;
        buf_size = &sessions[session_id].buf_out_size;
        buf_sent = &sessions[session_id].buf_out_sent;
    }

    normalize_buffer(buf, buf_size, buf_sent);
    if (*buf_size >= MAXLINE) {
        update_events(&sessions[session_id]);
        return 0;
    }

    bytes_read = recv(current_fd, buf + *buf_size, MAXLINE - *buf_size, 0);
    if (bytes_read <= 0) {
        clear_connection(sessions, session_id);
        return -1;
    }

    *buf_size += bytes_read;
    update_events(&sessions[session_id]);
    return 0;
}

int event_send(session *sessions, int index) {
    int current_fd = fds[index].fd;
    int session_id = find_session_by_fd(sessions, current_fd);
    char *buf;
    int *buf_size;
    int *buf_sent;
    ssize_t bytes_sent;
    int flags = 0;

    if (session_id == -1) return -1;

    if (sessions[session_id].sock_in == current_fd) {
        buf = sessions[session_id].buf_out;
        buf_size = &sessions[session_id].buf_out_size;
        buf_sent = &sessions[session_id].buf_out_sent;
    } else {
        buf = sessions[session_id].buf_in;
        buf_size = &sessions[session_id].buf_in_size;
        buf_sent = &sessions[session_id].buf_in_sent;
    }

    if (*buf_size == 0) {
        update_events(&sessions[session_id]);
        return 0;
    }

    bytes_sent = send(current_fd, buf + *buf_sent, *buf_size - *buf_sent, flags);
    if (bytes_sent <= 0) {
        clear_connection(sessions, session_id);
        return -1;
    }

    *buf_sent += bytes_sent;
    normalize_buffer(buf, buf_size, buf_sent);
    update_events(&sessions[session_id]);
    return 0;
}

int main(int argc, char *argv[]) {
    session *sessions;
    int port_in;
    char *address_out;
    int port_out;
    int listen_sock;
    int i;

    if (argc != 4) {
        fprintf(stderr, "usage: %s port host port_out\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);

    sessions = malloc(sizeof(session) * MAX_CONNECTIONS);
    if (sessions == NULL) {
        return 1;
    }

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        sessions[i].sock_in = -1;
        sessions[i].sock_out = -1;
        sessions[i].fd_in_index = -1;
        sessions[i].fd_out_index = -1;
        sessions[i].buf_in_size = 0;
        sessions[i].buf_in_sent = 0;
        sessions[i].buf_out_size = 0;
        sessions[i].buf_out_sent = 0;
    }

    for (i = 0; i < MAX_FDS; i++) {
        clear_fd_slot(i);
    }

    port_in = atoi(argv[1]);
    address_out = argv[2];
    port_out = atoi(argv[3]);

    listen_sock = get_listen_fd(port_in);
    if (listen_sock < 0) {
        free(sessions);
        return 1;
    }

    fds[0].fd = listen_sock;
    fds[0].events = POLLIN;

    while (!interrupt_flag) {
        int ret = poll(fds, MAX_FDS, -1);

        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll error");
            break;
        }

        if (fds[0].revents & POLLIN) {
            create_connection(sessions, address_out, port_out, listen_sock);
        }

        for (i = 1; i < MAX_FDS; i++) {
            int session_id;

            if (fds[i].fd == -1) continue;

            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                session_id = find_session_by_fd(sessions, fds[i].fd);
                if (session_id != -1) {
                    clear_connection(sessions, session_id);
                }
                continue;
            }

            if (fds[i].fd == -1) continue;
            if (fds[i].revents & POLLIN) event_data(sessions, i);
            if (fds[i].fd == -1) continue;
            if (fds[i].revents & POLLOUT) event_send(sessions, i);
        }
    }

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        clear_connection(sessions, i);
    }

    close(listen_sock);
    free(sessions);
    return 0;
}
