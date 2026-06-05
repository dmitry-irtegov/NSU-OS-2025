#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "terminal.h"
#include "url.h"

#define BUF_SIZE 4096
#define SCREEN_LINES 25
#define PROMPT "Press space to scroll down."

int connect_to_server(url_t* url) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res;
    struct addrinfo* rp;
    int error = getaddrinfo(url->host, url->port, &hints, &res);
    if (error != 0) {
        fprintf(stderr, "Error: cannot resolve %s\n", url->host);
        return -1;
    }

    int server = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        server = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server < 0) break;

        if (connect(server, rp->ai_addr, rp->ai_addrlen) == 0) break;

        close(server);
        server = -1;
    }
    freeaddrinfo(res);

    if (server < 0) {
        fprintf(stderr, "Error: cannot connect to %s:%s\n", url->host,
                url->port);
    }

    return server;
}

int send_request(url_t* url, int server) {
    char hostport[300];
    if (strcmp(url->port, "80") == 0) {
        snprintf(hostport, sizeof(hostport), "%s", url->host);
    } else {
        snprintf(hostport, sizeof(hostport), "%s:%s", url->host, url->port);
    }

    char request[BUF_SIZE];
    int request_len = snprintf(request, sizeof(request),
                               "GET %s HTTP/1.0\r\n"
                               "Host: %s\r\n"
                               "Connection: close\r\n"
                               "\r\n",
                               url->path, hostport);
    if (request_len < 0 || request_len >= (int)sizeof(request)) {
        return -1;
    }

    size_t total = (size_t)request_len;
    size_t sent = 0;
    while (sent < total) {
        ssize_t sent_now = send(server, request + sent, total - sent, 0);
        if (sent_now < 0) {
            return -1;
        }
        sent += (size_t)sent_now;
    }

    return 0;
}

int find_body_start(char* buf, size_t len, size_t* body_pos) {
    for (size_t i = 0; i + 4 <= len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' &&
            buf[i + 3] == '\n') {
            *body_pos = i + 4;
            return 1;
        }
    }
    return 0;
}

void http_client(url_t* url) {
    int server = connect_to_server(url);
    if (server < 0) {
        exit(1);
    }

    if (send_request(url, server) < 0) {
        perror("send_request");
        close(server);
        exit(1);
    }

    terminal_init();

    char* buf = NULL;
    size_t cap = 0;
    size_t len = 0;
    size_t pos = 0;
    int lines = 0;
    int paused = 0;
    int eof = 0;
    int headers_done = 0;

    while (!eof || pos < len) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;
        if (!eof) {
            FD_SET(server, &read_fds);
            max_fd = server;
        }
        if (paused) {
            FD_SET(STDIN_FILENO, &read_fds);
            if (STDIN_FILENO > max_fd) {
                max_fd = STDIN_FILENO;
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (!eof && FD_ISSET(server, &read_fds)) {
            if (len + BUF_SIZE > cap) {
                size_t new_cap = cap ? cap * 2 : 2 * BUF_SIZE;
                char* new_buf = realloc(buf, new_cap);
                if (new_buf == NULL) {
                    fprintf(stderr, "Error allocating memory\n");
                    break;
                }
                buf = new_buf;
                cap = new_cap;
            }

            ssize_t received = recv(server, buf + len, cap - len, 0);
            if (received > 0) {
                len += (size_t)received;
                if (!headers_done && find_body_start(buf, len, &pos)) {
                    headers_done = 1;
                }
            } else if (received == 0) {
                eof = 1;
            }
        }

        if (paused && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char key;
            if (read(STDIN_FILENO, &key, 1) == 1 && key == ' ') {
                fputs("\r\033[2K", stderr);
                paused = 0;
                lines = 0;
            }
        }

        while (headers_done && !paused && pos < len) {
            size_t end = pos;
            while (end < len && buf[end] != '\n') {
                end++;
            }
            int has_newline = end < len;
            size_t to = has_newline ? end + 1 : len;
            fwrite(buf + pos, 1, to - pos, stdout);
            pos = to;
            if (has_newline && ++lines >= SCREEN_LINES) {
                paused = 1;
                fflush(stdout);
                fputs(PROMPT, stderr);
            }
        }
        fflush(stdout);

        if (pos == len) {
            pos = 0;
            len = 0;
        }
        if (eof && !headers_done) {
            break;
        }
    }

    free(buf);
    shutdown(server, SHUT_RDWR);
    close(server);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Use: %s <URL>\n", argv[0]);
        return 1;
    }

    url_t url;
    if (url_parser(argv[1], &url) != 0) {
        fprintf(stderr, "Error: '%s' is not a valid URL\n", argv[1]);
        return 1;
    }

    http_client(&url);

    printf("\n");

    return 0;
}
