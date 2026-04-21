#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <termios.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "buffer.h"

#define MAX_LINES 25
#define CHUNK_SIZE 4096

struct termios oldt;

void reset_terminal() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0) {
        perror("tcsetattr");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return 1;
    }

    char *host = strstr(argv[1], "://");
    if (host) {
        host += 3;
    } else {
        host = argv[1];
    }

    char *path = strchr(host, '/');
    if (path) {
        *path = '\0';
        path++;
    } else {
        path = "";
    }

    char *port = strchr(host, ':');
    long port_num = 0;
    if (port) {
        *port = '\0';
        port++;
        port_num = atol(port);
        if (port_num <= 0 || port_num > 65535) {
            fprintf(stderr, "wrong port %ld\n", port_num);
            return 1;
        }
    } else {
        port_num = 80;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port_num)
    };
    
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        struct hostent *he = gethostbyname(host);
        if (!he) {
            fprintf(stderr, "gethostbyname: %s\n", hstrerror(h_errno));
            return 1;
        }

        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("connect");
        return 1;
    }

    char request[46 + strlen(path) + strlen(host)];
    int req_len = snprintf(request, sizeof(request), "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    if (send(sock, request, req_len, 0) < 0) {
        perror("send");
        return 1;
    }

    struct termios newt;
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        perror("tcgetattr");
        return 1;
    }
    if (atexit(reset_terminal) != 0) {
        perror("failed setup atexit");
        return 1;
    }
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
        perror("tcsetattr");
        return 1;
    }

    buffer content;
    buffer_init(&content, CHUNK_SIZE);
    int line_end = 0;
    int lines_count = 0;
    int sock_open = 1;
    int pos_sep = 0;
    const char *header_sep = "\r\n\r\n";

    fd_set rfds;
    FD_ZERO(&rfds);
    while (sock_open || content.size > 0) {
        if (sock_open) {
            FD_SET(sock, &rfds);
        }
        FD_SET(STDIN_FILENO, &rfds);

        if (select(sock + 1, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            return 1;
        }

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char c;
            int is_readed = read(STDIN_FILENO, &c, 1);
            if (is_readed < 0) {
                perror("read");
                return 1;
            } else if (is_readed == 0) {
                break;
            }

            if (c == 'q') {
                if (lines_count == MAX_LINES) {
                    printf("\r                                                       \r");
                    fflush(stdout);
                }
                return 0;
            } else if (c == 'p' && lines_count == MAX_LINES) {
                printf("\r                                                       \r");
                fflush(stdout);
                lines_count = 0;
            } else if (c == ' ' && lines_count == MAX_LINES) {
                printf("\r                                                       \r");
                fflush(stdout);
                lines_count--;
            }
        }

        if (sock_open && FD_ISSET(sock, &rfds)) {
            int n = buffer_recv(sock, &content, CHUNK_SIZE);
            if (n == 0) {
                sock_open = 0;
                FD_CLR(sock, &rfds);
            }
        }

        while (lines_count < MAX_LINES && content.size > 0) {
            if (pos_sep < 4) {
                char c = buffer_getchar(&content);
                if (c == header_sep[pos_sep]) {
                    pos_sep++;
                } else if (pos_sep > 0) {
                    pos_sep = 0;
                    if (c == header_sep[pos_sep]) {
                        pos_sep++;
                    }
                }
                continue;
            }
            
            while (content.buf[content.offset + line_end] != '\n' && line_end < (content.size - 1)) {
                line_end++;
            }

            if (content.buf[content.offset + line_end] != '\n' && line_end == (content.size - 1) && sock_open) {
                break;
            }

            int line_len = line_end + 1;
            int n = buffer_write(STDOUT_FILENO, &content, line_len);
            if (n == line_len) {
                lines_count++;
                line_end = 0;
            } else {
                line_end = line_len - n - 1;
            }
            if (lines_count >= MAX_LINES && content.size > 0) {
                printf("-- (space to next line) (p to next page) (q to quit) --");
                fflush(stdout);
            }
        }
    }

    return 0;
}
