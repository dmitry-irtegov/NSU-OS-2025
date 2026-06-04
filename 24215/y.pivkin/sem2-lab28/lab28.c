#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <termios.h>

#define BUF_SIZE    4096
#define PAGE_LINES  25

typedef struct {
    char    host[256];
    char    path[256];
    int     port;
} URLI;



void url_parser(const char *url, URLI *output) {
    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "Only http URLs are supported.\n");
        exit(2);
    }

    char *host_ptr = (char*)url + 7;
    char *path_ptr = strchr(host_ptr, '/');
    char *port_ptr = strchr(host_ptr, ':');

    output->port = 80;
    if (port_ptr != NULL && (path_ptr == NULL || port_ptr < path_ptr)) {
        output->port = atoi(port_ptr + 1);

        int n = port_ptr - host_ptr;
        strncpy(output->host, host_ptr, n);
        output->host[n] = '\0';

        if (path_ptr == NULL)
            strcpy(output->path, "/");
        else
            strcpy(output->path, path_ptr);
    } else {
        if (path_ptr == NULL) {
            strcpy(output->host, host_ptr);
            strcpy(output->path, "/");
        } else {
            int n = path_ptr - host_ptr;
            strncpy(output->host, host_ptr, n);
            output->host[n] = '\0';

            strcpy(output->path, path_ptr);
        }
    }
}



int connect_to_host(const char *host, int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        exit(3);
    }

    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "No such host: %s\n", host);
        close(s);
        exit(4);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(port);

    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(s);
        exit(5);
    }

    return s;
}



void send_request(int s, const char *host, const char *path) {
    char request[512];
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);

    if (send(s, request, strlen(request), 0) < 0) {
        perror("send");
        close(s);
        exit(6);
    }
}



void set_raw(int enable) {
    static struct termios saved;

    if (enable) {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        saved = t;

        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    } else
        tcsetattr(STDIN_FILENO, TCSANOW, &saved);
}



void waiting_space(int s, char **pending, int *len, int *cap) {
    printf("Press space to scroll down");
    fflush(stdout);
    set_raw(1);

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(s, &fds);

        int nfds = (s > STDIN_FILENO ? s : STDIN_FILENO) + 1;
        select(nfds, &fds, NULL, NULL, NULL);

        if (FD_ISSET(s, &fds)) {
            if (*len + BUF_SIZE > *cap) {
                *cap *= 2;
                *pending = realloc(*pending, *cap);
            }

            int n = recv(s, *pending + *len, BUF_SIZE, 0);
            if (n > 0)
                *len += n;
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            char c;
            read(STDIN_FILENO, &c, 1);
            if (c == ' ')
                break;
        }
    }

    set_raw(0);
    printf("\r                          \r");
    fflush(stdout);
}



void check_reply(int s) {
    int cap = BUF_SIZE * 16;
    char *pending = malloc(cap);
    int pending_len = 0;

    char buf[BUF_SIZE];
    int lines = 0;
    int print_pos = 0;

    int headers_pass = 0;
    int sock_closed = 0;

    int cut_symbols = 0;

    while (1) {
        if (print_pos >= pending_len) {
            if (sock_closed)
                break;

            int n = recv(s, buf, BUF_SIZE - 1, 0);
            if (n <= 0) {
                sock_closed = 1;
                break;
            }

            if (!headers_pass) {
                int body, found = 0;

                for (int i = 0; i < n; i++) {
                    if (cut_symbols == 0 && buf[i] == '\r')
                        cut_symbols = 1;
                    else if (cut_symbols == 1 && buf[i] == '\n')
                        cut_symbols = 2;
                    else if (cut_symbols == 2 && buf[i] == '\r')
                        cut_symbols = 3;
                    else if (cut_symbols == 3 && buf[i] == '\n') {
                        found = 1;
                        body = i + 1;

                        break;
                    } else {
                        if (buf[i] == '\r')
                            cut_symbols = 1;
                        else
                            cut_symbols = 0;
                    }
                }

                if (found) {
                    int len = n - body;
                    if (pending_len + len > cap) {
                        cap *= 2;
                        pending = realloc(pending, cap);
                    }

                    memcpy(pending + pending_len, buf + body, len);
                    pending_len += len;
                    headers_pass = 1;
                }
            } else {
                if (pending_len + n > cap) {
                    cap *= 2;
                    pending = realloc(pending, cap);
                }

                memcpy(pending + pending_len, buf, n);
                pending_len += n;
            }
        }

        while (print_pos < pending_len) {
            char c = pending[print_pos++];
            putchar(c);

            if (c == '\n') {
                lines++;

                if (lines >= PAGE_LINES && !sock_closed) {
                    lines = 0;
                    fflush(stdout);

                    waiting_space(s, &pending, &pending_len, &cap);
                }
            }
        }
    }

    fflush(stdout);
    free(pending);
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        exit(1);
    }

    URLI url;
    url_parser(argv[1], &url);

    printf("Connecting to %s:%d%s\n\n", url.host, url.port, url.path);
    int s = connect_to_host(url.host, url.port);
    send_request(s, url.host, url.path);

    check_reply(s);
    close(s);

    printf("\n");
    return 0;
}
