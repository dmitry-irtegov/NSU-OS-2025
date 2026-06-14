#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define SCREEN_LINES 25

struct termios oldt;

void restore_terminal(void) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0) {
        perror("tcsetattr");
    }
}

int setup_terminal(void) {
    struct termios newt;

    if (!isatty(STDIN_FILENO)) {
        return -1;
    }
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        return -1;
    }
    if (atexit(restore_terminal) != 0) {
        return -1;
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
        return -1;
    }

    return 0;
}

int send_request(int sockfd, const char *request) {
    size_t sent = 0;
    size_t length = strlen(request);

    while (sent < length) {
        ssize_t n = send(sockfd, request + sent, length - sent, 0);

        if (n > 0) {
            sent += (size_t)n;
        } else if (n == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s http://host[:port]/path\n", argv[0]);
        return 1;
    }

    if (strncmp(argv[1], "http://", 7) != 0) {
        fprintf(stderr, "Invalid URL\n");
        return 1;
    }

    char *url = strdup(argv[1] + 7);
    if (url == NULL) {
        perror("strdup");
        return 1;
    }

    char *path = strchr(url, '/');
    if (path != NULL) {
        *path = '\0';
        path++;
    } else {
        path = "";
    }

    char *host_header = strdup(url);
    if (host_header == NULL) {
        perror("strdup");
        free(url);
        return 1;
    }

    char *host = url;
    char *port = strrchr(host, ':');
    if (port != NULL) {
        *port = '\0';
        port++;

        char *end;
        long port_number = strtol(port, &end, 10);
        if (*host == '\0' || *end != '\0' ||
            port_number < 1 || port_number > 65535) {
            fprintf(stderr, "Invalid port\n");
            free(host_header);
            free(url);
            return 1;
        }
    } else {
        port = "80";
    }

    if (*host == '\0') {
        fprintf(stderr, "Invalid host\n");
        free(host_header);
        free(url);
        return 1;
    }

    struct addrinfo hints = {0};
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int error = getaddrinfo(host, port, &hints, &res);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        free(host_header);
        free(url);
        return 1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        free(host_header);
        free(url);
        return 1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(sockfd);
        free(host_header);
        free(url);
        return 1;
    }
    freeaddrinfo(res);

    int request_length = snprintf(NULL, 0,
                                  "GET /%s HTTP/1.0\r\n"
                                  "Host: %s\r\n"
                                  "Connection: close\r\n\r\n",
                                  path, host_header);
    if (request_length < 0) {
        fprintf(stderr, "Cannot create HTTP request\n");
        close(sockfd);
        free(host_header);
        free(url);
        return 1;
    }

    char *request = malloc((size_t)request_length + 1);
    if (request == NULL) {
        perror("malloc");
        close(sockfd);
        free(host_header);
        free(url);
        return 1;
    }

    snprintf(request, (size_t)request_length + 1,
             "GET /%s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, host_header);

    if (send_request(sockfd, request) == -1) {
        perror("send");
        free(request);
        close(sockfd);
        free(host_header);
        free(url);
        return 1;
    }
    free(request);
    free(host_header);
    free(url);

    if (setup_terminal() == -1) {
        perror("terminal");
        close(sockfd);
        return 1;
    }

    size_t capacity = BUF_SIZE;
    size_t bytes_in_buffer = 0;
    size_t buffer_pos = 0;
    char *buffer = malloc(capacity);

    if (buffer == NULL) {
        perror("malloc");
        close(sockfd);
        return 1;
    }

    bool headers_skipped = false;
    bool paused = false;
    bool connection_closed = false;

    const char separator[] = "\r\n\r\n";
    int separator_pos = 0;
    int lines = 0;
    int result = 0;

    while (!connection_closed || buffer_pos < bytes_in_buffer) {
        if (buffer_pos > 0) {
            size_t remain = bytes_in_buffer - buffer_pos;
            memmove(buffer, buffer + buffer_pos, remain);
            bytes_in_buffer = remain;
            buffer_pos = 0;
        }

        if (!connection_closed && capacity - bytes_in_buffer < BUF_SIZE) {
            char *new_buffer = realloc(buffer, capacity * 2);

            if (new_buffer == NULL) {
                perror("realloc");
                result = 1;
                break;
            }

            buffer = new_buffer;
            capacity *= 2;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (!connection_closed) {
            FD_SET(sockfd, &readfds);
        }

        int maxfd = connection_closed ? STDIN_FILENO : sockfd;
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("select");
            result = 1;
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char c;

            if (read(STDIN_FILENO, &c, 1) <= 0) {
                break;
            }

            if (paused && c == ' ') {
                paused = false;
                lines = 0;
                printf("\r                               \r");
                fflush(stdout);
            }
        }

        if (!connection_closed && FD_ISSET(sockfd, &readfds)) {
            ssize_t n = recv(sockfd,
                             buffer + bytes_in_buffer,
                             capacity - bytes_in_buffer,
                             0);

            if (n > 0) {
                bytes_in_buffer += (size_t)n;
            } else if (n == 0) {
                connection_closed = true;
            } else if (errno != EINTR) {
                perror("recv");
                result = 1;
                break;
            }
        }

        while (!headers_skipped && buffer_pos < bytes_in_buffer) {
            char c = buffer[buffer_pos++];

            if (c == separator[separator_pos]) {
                separator_pos++;
            } else if (c == separator[0]) {
                separator_pos = 1;
            } else {
                separator_pos = 0;
            }

            if (separator_pos == 4) {
                headers_skipped = true;
            }
        }

        while (headers_skipped && !paused && buffer_pos < bytes_in_buffer) {
            char c = buffer[buffer_pos++];

            if (putchar((unsigned char)c) == EOF) {
                result = 1;
                break;
            }

            if (c == '\n') {
                lines++;
            }
            if (lines >= SCREEN_LINES) {
                paused = true;
                printf("\nPress space to scroll down");
                fflush(stdout);
            }
        }

        if (result != 0) {
            break;
        }

        fflush(stdout);
    }

    free(buffer);
    close(sockfd);
    return result;
}
