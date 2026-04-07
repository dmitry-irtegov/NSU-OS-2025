#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define LINES_PER_PAGE 25

typedef struct {
    char host[256];
    char port[6];
    char path[1024];
} url_t;

void parse_url(const char *url, url_t *result) {
    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "Only http:// supported\n");
        exit(1);
    }

    const char *p = url + 7;
    const char *slash = strchr(p, '/');

    if (slash) {
        strncpy(result->host, p, slash - p);
        result->host[slash - p] = '\0';
        strcpy(result->path, slash);
    } else {
        strcpy(result->host, p);
        strcpy(result->path, "/");
    }

    strcpy(result->port, "80");

    char *colon = strchr(result->host, ':');
    if (colon) {
        strcpy(result->port, colon + 1);
        *colon = '\0';
    }
}

int connect_to_host(const char *host, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd = -1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            sockfd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "Failed to connect\n");
        return -1;
    }

    return sockfd;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return 1;
    }

    url_t url;
    parse_url(argv[1], &url);

    int sockfd = connect_to_host(url.host, url.port);
    if (sockfd < 0) return 1;

    char request[2048];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",
        url.path, url.host);

    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }

    fd_set readfds;
    char recv_buf[BUFFER_SIZE];

    char *data = NULL;
    size_t data_size = 0;
    size_t printed = 0;

    int lines = 0;
    int paused = 0;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            int bytes = recv(sockfd, recv_buf, BUFFER_SIZE, 0);
            if (bytes < 0) {
                perror("recv");
                break;
            }
            if (bytes == 0) {
                break;
            }

            char *tmp = realloc(data, data_size + bytes);
            if (!tmp) {
                perror("realloc");
                free(data);
                close(sockfd);
                return 1;
            }

            data = tmp;
            memcpy(data + data_size, recv_buf, bytes);
            data_size += bytes;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                if (c == ' ') {
                    paused = 0;
                    lines = 0;
                }
            }
        }

        while (printed < data_size && !paused) {
            char c = data[printed++];
            putchar(c);

            if (c == '\n') {
                lines++;
                if (lines >= LINES_PER_PAGE) {
                    paused = 1;
                    printf("Press space to scroll down\n");
                    fflush(stdout);
                    break;
                }
            }
        }
    }

    while (printed < data_size) {
        putchar(data[printed++]);
    }

    free(data);
    close(sockfd);
    return 0;
}

