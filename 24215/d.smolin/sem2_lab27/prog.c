#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_SESSIONS 510
#define BUF_SIZE 4096

typedef struct {
    unsigned char data[BUF_SIZE];
    int used;
} buffer_t;

typedef struct {
    int client_fd;
    int server_fd;
    buffer_t c2s;
    buffer_t s2c;
    char client_read_eof;
    char server_read_eof;
    char client_write_shutdown;
    char server_write_shutdown;
} session_t;

static session_t *sessions[MAX_SESSIONS];
static int n_sessions = 0;
static volatile sig_atomic_t stop_flag = 0;

static void on_signal(int sig) {
    (void)sig;
    stop_flag = 1;
}

static int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    if (listen(fd, 64) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

static int connect_to_target(const char *host, const char *port_str) {
    struct addrinfo hints, *res = NULL, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        return -1;
    }

    int fd = -1;
    for (ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

static void close_session(session_t *s) {
    close(s->client_fd);
    close(s->server_fd);
    free(s);
}

static void buf_drain(buffer_t *b, int n) {
    if (n <= 0) return;
    if (n >= b->used) {
        b->used = 0;
    } else {
        memmove(b->data, b->data + n, b->used - n);
        b->used -= n;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <listen_port> <target_host> <target_port>\n", argv[0]);
        return 1;
    }

    int listen_port = atoi(argv[1]);
    const char *target_host = argv[2];
    const char *target_port = argv[3];

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rlim_t want = 1024;
        if (rl.rlim_cur < want) {
            rl.rlim_cur = (rl.rlim_max < want && rl.rlim_max != RLIM_INFINITY)
                          ? rl.rlim_max : want;
            setrlimit(RLIMIT_NOFILE, &rl);
        }
    }

    int listen_fd = make_listen_socket(listen_port);
    if (listen_fd < 0) return 1;

    fprintf(stderr, "Listening on port %d, forwarding to %s:%s\n",
            listen_port, target_host, target_port);

    static struct pollfd pfds[1 + MAX_SESSIONS * 2];

    while (!stop_flag) {
        int nfds = 0;
        pfds[nfds].fd = listen_fd;
        pfds[nfds].events = (n_sessions < MAX_SESSIONS) ? POLLIN : 0;
        pfds[nfds].revents = 0;
        nfds++;

        for (int i = 0; i < n_sessions; i++) {
            session_t *s = sessions[i];
            short cev = 0, sev = 0;
            if (!s->client_read_eof && s->c2s.used < BUF_SIZE) cev |= POLLIN;
            if (s->s2c.used > 0)                               cev |= POLLOUT;
            if (!s->server_read_eof && s->s2c.used < BUF_SIZE) sev |= POLLIN;
            if (s->c2s.used > 0)                               sev |= POLLOUT;

            pfds[nfds].fd = s->client_fd;
            pfds[nfds].events = cev;
            pfds[nfds].revents = 0;
            nfds++;
            pfds[nfds].fd = s->server_fd;
            pfds[nfds].events = sev;
            pfds[nfds].revents = 0;
            nfds++;
        }

        int r = poll(pfds, nfds, -1);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        for (int i = 0; i < n_sessions; ) {
            session_t *s = sessions[i];
            int pi_c = 1 + i * 2;
            int pi_s = 1 + i * 2 + 1;
            short cre = pfds[pi_c].revents;
            short sre = pfds[pi_s].revents;
            int kill = 0;

            if ((cre & POLLIN) && !s->client_read_eof && s->c2s.used < BUF_SIZE) {
                int n = read(s->client_fd,
                             s->c2s.data + s->c2s.used,
                             BUF_SIZE - s->c2s.used);
                if (n > 0)        s->c2s.used += n;
                else if (n == 0)  s->client_read_eof = 1;
                else if (errno != EINTR) kill = 1;
            }
            if (!kill && (sre & POLLIN) && !s->server_read_eof && s->s2c.used < BUF_SIZE) {
                int n = read(s->server_fd,
                             s->s2c.data + s->s2c.used,
                             BUF_SIZE - s->s2c.used);
                if (n > 0)        s->s2c.used += n;
                else if (n == 0)  s->server_read_eof = 1;
                else if (errno != EINTR) kill = 1;
            }

            if (!kill && (cre & POLLOUT) && s->s2c.used > 0) {
                int n = write(s->client_fd, s->s2c.data, s->s2c.used);
                if (n > 0) buf_drain(&s->s2c, n);
                else if (n < 0 && errno != EINTR) kill = 1;
            }
            if (!kill && (sre & POLLOUT) && s->c2s.used > 0) {
                int n = write(s->server_fd, s->c2s.data, s->c2s.used);
                if (n > 0) buf_drain(&s->c2s, n);
                else if (n < 0 && errno != EINTR) kill = 1;
            }

            if ((cre | sre) & (POLLERR | POLLNVAL)) kill = 1;

            if (!kill) {
                if (s->client_read_eof && s->c2s.used == 0 && !s->server_write_shutdown) {
                    shutdown(s->server_fd, SHUT_WR);
                    s->server_write_shutdown = 1;
                }
                if (s->server_read_eof && s->s2c.used == 0 && !s->client_write_shutdown) {
                    shutdown(s->client_fd, SHUT_WR);
                    s->client_write_shutdown = 1;
                }
                if (s->client_read_eof && s->server_read_eof
                    && s->c2s.used == 0 && s->s2c.used == 0) {
                    kill = 1;
                }
            }

            if (kill) {
                close_session(s);
                int last = --n_sessions;
                if (i != last) {
                    sessions[i] = sessions[last];
                    pfds[1 + i * 2]     = pfds[1 + last * 2];
                    pfds[1 + i * 2 + 1] = pfds[1 + last * 2 + 1];
                }
            } else {
                i++;
            }
        }

        if (pfds[0].revents & POLLIN) {
            int cfd = accept(listen_fd, NULL, NULL);
            if (cfd >= 0) {
                if (n_sessions >= MAX_SESSIONS) {
                    close(cfd);
                } else {
                    int sfd = connect_to_target(target_host, target_port);
                    if (sfd < 0) {
                        close(cfd);
                    } else {
                        session_t *s = calloc(1, sizeof(*s));
                        if (!s) {
                            close(cfd);
                            close(sfd);
                        } else {
                            s->client_fd = cfd;
                            s->server_fd = sfd;
                            sessions[n_sessions++] = s;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < n_sessions; i++) close_session(sessions[i]);
    close(listen_fd);
    return 0;
}
