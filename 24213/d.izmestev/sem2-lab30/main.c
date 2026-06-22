#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>

#define BUF_SIZE 4096
#define INITIAL_LINES 25

typedef struct {
    char buf[BUF_SIZE];
    int head;
    int tail;
    int empty;
    int full;
    int eof;
    int sockfd;
    int quit;

    pthread_mutex_t mutex;
    pthread_cond_t cond_pro;
    pthread_cond_t cond_con;
} shared_ctx_t;

struct termios orig_termios;

void check_pthread(int res, const char *msg) {
    if (res != 0) {
        fprintf(stderr, "%s: %s\n", msg, strerror(res));
        exit(EXIT_FAILURE);
    }
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) {
        perror("tcgetattr"); exit(EXIT_FAILURE);
    }
    if (atexit(disable_raw_mode) != 0) {
        fprintf(stderr, "atexit failed\n"); exit(EXIT_FAILURE);
    }
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) {
        perror("tcsetattr"); exit(EXIT_FAILURE);
    }
}

void parse_url(const char *url, char **hostname, char **port, char **path) {
    char *url_copy = strdup(url);
    if (!url_copy) { perror("strdup url"); exit(EXIT_FAILURE); }

    char *p = url_copy;
    char *scheme = strstr(p, "://");
    if (scheme) p = scheme + 3;

    char *slash = strchr(p, '/');
    if (slash) {
        *path = strdup(slash);
        if (!*path) { perror("strdup path"); exit(EXIT_FAILURE); }
        *slash = '\0';
    } else {
        *path = strdup("/");
        if (!*path) { perror("strdup path"); exit(EXIT_FAILURE); }
    }

    char *colon = strchr(p, ':');
    if (colon) {
        *colon = '\0';
        *port = strdup(colon + 1);
        if (!*port) { perror("strdup port"); exit(EXIT_FAILURE); }
    } else {
        *port = strdup("80");
        if (!*port) { perror("strdup port"); exit(EXIT_FAILURE); }
    }

    *hostname = strdup(p);
    if (!*hostname) { perror("strdup hostname"); exit(EXIT_FAILURE); }

    free(url_copy);
}

int getindafter(int start, int offset) {
    return (start + offset) % BUF_SIZE;
}

void *producer_func(void *arg) {
    shared_ctx_t *ctx = (shared_ctx_t *)arg;

    while (1) {
        check_pthread(pthread_mutex_lock(&ctx->mutex), "mutex_lock (pro)");

        while (ctx->full && !ctx->eof && !ctx->quit) {
            check_pthread(pthread_cond_wait(&ctx->cond_pro, &ctx->mutex), "cond_wait (pro)");
        }

        if (ctx->eof || ctx->quit) {
            check_pthread(pthread_mutex_unlock(&ctx->mutex), "mutex_unlock (pro)");
            break;
        }

        int space;
        if (ctx->tail >= ctx->head) {
            space = BUF_SIZE - ctx->tail;
        } else {
            space = ctx->head - ctx->tail;
        }

        int fd = ctx->sockfd;
        int current_tail = ctx->tail;

        check_pthread(pthread_mutex_unlock(&ctx->mutex), "mutex_unlock (pro)");
        ssize_t n = recv(fd, ctx->buf + current_tail, space, 0);
        check_pthread(pthread_mutex_lock(&ctx->mutex), "mutex_lock (pro)");

        if (n <= 0) {
            ctx->eof = 1;
            if (n < 0) perror("recv");
            check_pthread(pthread_cond_broadcast(&ctx->cond_con), "cond_broadcast");
            check_pthread(pthread_mutex_unlock(&ctx->mutex), "mutex_unlock (pro)");
            break;
        }

        ctx->tail = getindafter(ctx->tail, (int)n);
        ctx->empty = 0;
        if (ctx->tail == ctx->head) ctx->full = 1;

        check_pthread(pthread_cond_signal(&ctx->cond_con), "cond_signal (pro)");
        check_pthread(pthread_mutex_unlock(&ctx->mutex), "mutex_unlock (pro)");
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    char *hostname = NULL;
    char *path = NULL;
    char *port = NULL;

    parse_url(argv[1], &hostname, &port, &path);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(hostname, port, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(err));
        free(hostname); free(path); free(port);
        return EXIT_FAILURE;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res); free(hostname); free(path);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        close(sockfd); freeaddrinfo(res); free(hostname); free(path);
        return EXIT_FAILURE;
    }

    freeaddrinfo(res);

    const char *fmt = "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n";
    int req_len = snprintf(NULL, 0, fmt, path, hostname);
    if (req_len < 0) { perror("snprintf size"); return EXIT_FAILURE; }

    char *req = malloc(req_len + 1);
    if (!req) { perror("malloc req"); return EXIT_FAILURE; }
    snprintf(req, req_len + 1, fmt, path, hostname);

    if (write(sockfd, req, req_len) < 0) {
        perror("write request");
        free(req); free(hostname); free(path);
        return EXIT_FAILURE;
    }

    free(hostname);
    free(path);
    free(port);
    free(req);

    shared_ctx_t ctx;
    ctx.head = 0;
    ctx.tail = 0;
    ctx.empty = 1;
    ctx.full = 0;
    ctx.eof = 0;
    ctx.sockfd = sockfd;
    ctx.quit = 0;
    check_pthread(pthread_mutex_init(&ctx.mutex, NULL), "mutex_init");
    check_pthread(pthread_cond_init(&ctx.cond_pro, NULL), "cond_pro_init");
    check_pthread(pthread_cond_init(&ctx.cond_con, NULL), "cond_con_init");

    enable_raw_mode();

    pthread_t tid;
    check_pthread(pthread_create(&tid, NULL, producer_func, &ctx), "pthread_create");

    int lines_shown = 0, headers_ok = 0, h_state = 0;
    int last_processed_tail = -1;
    int need_wait = 0;

    while (1) {
        check_pthread(pthread_mutex_lock(&ctx.mutex), "mutex_lock (con)");

        if (ctx.quit) {
            check_pthread(pthread_mutex_unlock(&ctx.mutex), "mutex_unlock (con)");
            break;
        }

        while ((ctx.empty || (need_wait && ctx.tail == last_processed_tail)) && !ctx.eof) {
            check_pthread(pthread_cond_wait(&ctx.cond_con, &ctx.mutex), "cond_wait (con)");
        }
        last_processed_tail = ctx.tail;
        need_wait = 0;

        if (ctx.empty && ctx.eof) {
            check_pthread(pthread_mutex_unlock(&ctx.mutex), "mutex_unlock (con)");
            break;
        }

        int is_full = ctx.full;

        int avail;
        if (ctx.full) {
            avail = BUF_SIZE;
        } else if (ctx.tail >= ctx.head) {
            avail = ctx.tail - ctx.head;
        } else {
            avail = (BUF_SIZE - ctx.head) + ctx.tail;
        }

        check_pthread(pthread_mutex_unlock(&ctx.mutex), "mutex_unlock (con)");

        int start = 0;

        if (!headers_ok) {
            while (!headers_ok && start < avail) {
                char c = ctx.buf[getindafter(ctx.head, start)];
                if (c == '\r') h_state = (h_state == 0 || h_state == 2) ? h_state + 1 : 0;
                else if (c == '\n') h_state = (h_state == 1 || h_state == 3) ? h_state + 1 : 0;
                else h_state = 0;
                if (h_state == 4) headers_ok = 1;
                start++;
            }

            check_pthread(pthread_mutex_lock(&ctx.mutex), "mutex_lock");
            ctx.head = getindafter(ctx.head, start);
            ctx.full = 0; if (ctx.head == ctx.tail) ctx.empty = 1;
            check_pthread(pthread_cond_signal(&ctx.cond_pro), "cond_signal");
            check_pthread(pthread_mutex_unlock(&ctx.mutex), "mutex_unlock");

            if (!headers_ok) {
                continue;
            } else {
                avail -= start;
                start = 0;
            }
        }

        if (headers_ok && avail > 0) {
            int user_quit = 0;
            while (start < avail) {
                int end = start;
                int found_nl = 0;

                while (end < avail) {
                    if (ctx.buf[getindafter(ctx.head, end)] == '\n') {
                        found_nl = 1;
                        break;
                    }
                    end++;
                }

                if (!found_nl && !ctx.eof && (!is_full || (avail - start) != BUF_SIZE)) {
                    need_wait = 1;
                    break;
                }

                int len = end - start + found_nl;

                int chunk = BUF_SIZE - getindafter(ctx.head, start);
                if (chunk < len) {
                    if (write(STDOUT_FILENO, ctx.buf + getindafter(ctx.head, start), chunk) < 0) perror("write");
                    if (write(STDOUT_FILENO, ctx.buf, len - chunk) < 0) perror("write");
                } else {
                    if (write(STDOUT_FILENO, ctx.buf + getindafter(ctx.head, start), len) < 0) perror("write");
                }

                check_pthread(pthread_mutex_lock(&ctx.mutex), "mutex_lock");
                ctx.head = getindafter(ctx.head, len);
                ctx.full = 0; if (ctx.head == ctx.tail) ctx.empty = 1;
                check_pthread(pthread_cond_signal(&ctx.cond_pro), "cond_signal");
                check_pthread(pthread_mutex_unlock(&ctx.mutex), "mutex_unlock");

                if (found_nl) {
                    lines_shown++;
                    if (lines_shown >= INITIAL_LINES) {
                        const char *msg = " -- [ space/enter to continue, q to quit ] -- ";
                        if (write(STDOUT_FILENO, msg, strlen(msg)) < 0) perror("write");

                        char cmd;
                        while (read(STDIN_FILENO, &cmd, 1) > 0) {
                            if (cmd == 'q' || cmd == 'Q') {
                                if (write(STDOUT_FILENO, "\n", 1) < 0) perror("write \\n");
                                user_quit = 1;
                                break;
                            }
                            if (cmd == ' ' || cmd == '\n' || cmd == '\r') {
                                if (write(STDOUT_FILENO, "\r                                              \r", 48) < 0) perror("write clear");
                                lines_shown = INITIAL_LINES - 1;
                                break;
                            }
                        }
                    }
                }
                if (user_quit) {
                    break;
                }
                avail -= len;
                start = 0;
            }
            if (user_quit) {
                check_pthread(pthread_mutex_lock(&ctx.mutex), "mutex_lock (quit)");
                ctx.quit = 1;
                check_pthread(pthread_cond_broadcast(&ctx.cond_pro), "cond_broadcast (quit)");
                check_pthread(pthread_mutex_unlock(&ctx.mutex), "mutex_unlock (quit)");
                break;
            }
        }
    }

    check_pthread(pthread_join(tid, NULL), "pthread_join");
    check_pthread(pthread_mutex_destroy(&ctx.mutex), "mutex_destroy");
    check_pthread(pthread_cond_destroy(&ctx.cond_pro), "cond_pro_destroy");
    check_pthread(pthread_cond_destroy(&ctx.cond_con), "cond_con_destroy");
    close(sockfd);

    return EXIT_SUCCESS;
}
