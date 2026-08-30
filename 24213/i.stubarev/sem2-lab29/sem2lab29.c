#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <termios.h>
#include <arpa/inet.h>
#include <aio.h>
#include <errno.h>

#define MAX_LINES 25
#define BUF_SIZE 8192

typedef struct {
    char buf[BUF_SIZE];
    int head;
    int tail;
    int empty;
    int full;
} RingBuffer;

struct termios orig_term;

void restore_term() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_term) != 0) {
        perror("Failed tcsetattr");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return 1;
    }

    char *url = argv[1];
    char *hostname = strstr(url, "://");
    hostname = hostname ? hostname + 3 : url;

    char *uri_path = strchr(hostname, '/');
    if (uri_path) {
        *uri_path = '\0';
        uri_path++;
    } else {
        uri_path = "";
    }

    char *port_str = strchr(hostname, ':');
    int port_val = 80;
    if (port_str) {
        *port_str = '\0';
        port_str++;
        port_val = atoi(port_str);
        if (port_val <= 0 || port_val > 65535) {
            fprintf(stderr, "Wrong port\n");
            return 1;
        }
    }

    struct addrinfo hints, *res_addr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_buf[16];
    snprintf(port_buf, sizeof(port_buf), "%d", port_val);

    int status = getaddrinfo(hostname, port_buf, &hints, &res_addr);
    if (status != 0) {
        fprintf(stderr, "Failed getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    int sockfd = socket(res_addr->ai_family, res_addr->ai_socktype, res_addr->ai_protocol);
    if (sockfd < 0) {
        perror("Failed socket");
        freeaddrinfo(res_addr);
        return 1;
    }

    if (connect(sockfd, res_addr->ai_addr, res_addr->ai_addrlen) != 0) {
        perror("Failed connect");
        freeaddrinfo(res_addr);
        return 1;
    }
    freeaddrinfo(res_addr);

    char http_req[64 + strlen(uri_path) + strlen(hostname)];
    int req_size = snprintf(http_req, sizeof(http_req), "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", uri_path, hostname);
    if (send(sockfd, http_req, req_size, 0) < 0) {
        perror("Failed send");
        return 1;
    }

    if (tcgetattr(STDIN_FILENO, &orig_term) < 0) {
        perror("Failed tcgetattr");
        return 1;
    }

    if (atexit(restore_term) != 0) {
        fprintf(stderr, "Failed atexit\n");
        restore_term();
        return 1;
    }

    struct termios raw_term = orig_term;
    raw_term.c_lflag &= ~(ICANON | ECHO);
    raw_term.c_cc[VMIN] = 1;
    raw_term.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_term) < 0) {
        perror("Failed tcsetattr");
        return 1;
    }

    RingBuffer rb;
    memset(&rb, 0, sizeof(rb));
    rb.empty = 1;
    char term_buf;
    int eof = 0;
    int lines = 0;

    struct aiocb sock_read, term_write, term_read;

    memset(&sock_read, 0, sizeof(sock_read));
    sock_read.aio_fildes = sockfd;

    memset(&term_write, 0, sizeof(term_write));
    term_write.aio_fildes = STDOUT_FILENO;

    memset(&term_read, 0, sizeof(term_read));
    term_read.aio_fildes = STDIN_FILENO;
    term_read.aio_buf = &term_buf;

    int header_passed = 0;
    int header_idx = 0;
    const char *header_sep = "\r\n\r\n";
    int prompt_printed = 0;

    while (1) {
        struct aiocb *rqv[3] = { NULL };
        int rqt = 0;

        while (!header_passed && !rb.empty) {
            char c = rb.buf[rb.head];
            if (c == header_sep[header_idx]) {
                header_idx++;
            } else {
                header_idx = (c == '\r') ? 1 : 0;
            }
            if (header_idx == 4) {
                header_passed = 1;
            }
            rb.head++;
            if (rb.head == sizeof(rb.buf)) {
                rb.head = 0;
            }
            if (rb.head == rb.tail) {
                rb.empty = 1;
            }
            rb.full = 0;
        }

        if (!rb.full && !eof && sock_read.aio_nbytes == 0) {
            sock_read.aio_buf = rb.buf + rb.tail;
            sock_read.aio_nbytes = (rb.tail >= rb.head) ? (sizeof(rb.buf) - rb.tail) : (rb.head - rb.tail);
            if (aio_read(&sock_read) < 0) {
                perror("\nFailed aio_read network");
                return 1;
            }
        }
        if (sock_read.aio_nbytes > 0) {
            rqv[rqt++] = &sock_read;
        }

        if (header_passed && !rb.empty && lines < MAX_LINES && term_write.aio_nbytes == 0) {
            char *t = rb.buf + rb.head;
            char *l = (rb.head < rb.tail) ? (rb.buf + rb.tail) : (rb.buf + sizeof(rb.buf));

            for (; t < l && *t != '\n'; t++);
            if (t < l) {
                t++;
            }
            term_write.aio_buf = rb.buf + rb.head;
            term_write.aio_nbytes = t - (rb.buf + rb.head);
            if (aio_write(&term_write) < 0) {
                perror("\nFailed aio_write screen");
                return 1;
            }
        }
        if (term_write.aio_nbytes > 0) {
            rqv[rqt++] = &term_write;
        }

        if (lines >= MAX_LINES) {
            if (!prompt_printed) {
                const char *prompt = "-- (space to next line) (p to next page) (q to quit) --";
                if (write(STDOUT_FILENO, prompt, strlen(prompt)) < 0) {
                    perror("\nFailed write prompt");
                    return 1;
                }
                prompt_printed = 1;
            }
            if (term_read.aio_nbytes == 0) {
                term_read.aio_nbytes = 1;
                if (aio_read(&term_read) < 0) {
                    perror("\nFailed aio_read term");
                    return 1;
                }
            }
            rqv[rqt++] = &term_read;
        } else {
            term_read.aio_nbytes = 0;
        }

        if (eof && rb.empty && aio_error(&term_write) != EINPROGRESS) {
            break;
        }
        if (rqt == 0) break;

        if (aio_suspend((const struct aiocb **)rqv, rqt, NULL) < 0) {
            perror("\nFailed aio_suspend");
            return 1;
        }

        if (sock_read.aio_nbytes > 0 && aio_error(&sock_read) != EINPROGRESS) {
            int err = aio_error(&sock_read);
            if (err != 0) {
                fprintf(stderr, "\nNetwork error: %s\n", strerror(err));
                return 1;
            }

            ssize_t res = aio_return(&sock_read);
            if (res < 0) {
                perror("\nFailed aio_return network");
                return 1;
            }

            sock_read.aio_nbytes = 0;

            if (res == 0) {
                eof = 1;
            } else if (res > 0) {
                rb.tail += res;
                if (rb.tail == sizeof(rb.buf)) {
                    rb.tail = 0;
                }
                if (rb.tail == rb.head) {
                    rb.full = 1;
                }
                rb.empty = 0;
            }
        }

        if (term_write.aio_nbytes > 0 && aio_error(&term_write) != EINPROGRESS) {
            int err = aio_error(&term_write);
            if (err != 0) {
                fprintf(stderr, "\nScreen error: %s\n", strerror(err));
                return 1;
            }

            ssize_t res = aio_return(&term_write);
            if (res < 0) {
                perror("\nFailed aio_return screen");
                return 1;
            }

            term_write.aio_nbytes = 0;

            if (res > 0) {
                term_write.aio_offset += res;
                char *wb = (char *)term_write.aio_buf;
                if (wb[res - 1] == '\n') {
                    lines++;
                }
                rb.head += res;
                if (rb.head == sizeof(rb.buf)) {
                    rb.head = 0;
                }
                if (rb.head == rb.tail) {
                    rb.empty = 1;
                }
                rb.full = 0;
            }
        }

        if (term_read.aio_nbytes > 0 && aio_error(&term_read) != EINPROGRESS) {
            int err = aio_error(&term_read);
            if (err != 0) {
                fprintf(stderr, "\nTerminal error: %s\n", strerror(err));
                return 1;
            }

            ssize_t res = aio_return(&term_read);
            if (res < 0) {
                perror("\nFaild aio_return term");
                return 1;
            }

            term_read.aio_nbytes = 0;

            if (res > 0) {
                const char *clear_prompt = "\r                                                         \r";
                if (write(STDOUT_FILENO, clear_prompt, strlen(clear_prompt)) < 0) {
                    perror("\nFailed write clear_prompt");
                    return 1;
                }
                prompt_printed = 0;

                if (term_buf == 'q') {
                    break;
                } else if (term_buf == 'p') {
                    lines = 0;
                } else if (term_buf == ' ') {
                    lines--;
                }
            }
        }
    }
    return 0;
}
