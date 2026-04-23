#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>

#define MAX_LINES 25
#define BUF_SIZE 4096

struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(reset_terminal_mode);
    struct termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void parse_url(const char *url, char **host, char **port, char **path) {
    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) p += 7;
    
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');
    
    size_t host_len;
    if (colon && (!slash || colon < slash)) {
        host_len = colon - p;
        *port = strndup(colon + 1, slash ? (size_t)(slash - colon - 1) : strlen(colon + 1));
    } else {
        host_len = slash ? (size_t)(slash - p) : strlen(p);
        *port = strdup("80");
    }
    *host = strndup(p, host_len);
    *path = slash ? strdup(slash) : strdup("/");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <url>\n", argv[0]);
        return 1;
    }

    char *host, *port, *path;
    parse_url(argv[1], &host, &port, &path);

    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        return 1;
    }
    freeaddrinfo(res);

    char request[2048];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n", path, host);
    
    size_t req_len = strlen(request);
    size_t sent = 0;
    while (sent < req_len) {
        ssize_t w = write(sockfd, request + sent, req_len - sent);
        if (w <= 0) break;
        sent += w;
    }

    free(host);
    free(port); 
    free(path);

    set_conio_terminal_mode();

    int socket_open = 1;
    int is_paused = 0;
    int lines_printed = 0;

    int headers_skipped = 0;
    int match_count = 0; 

    char buf[BUF_SIZE];
    ssize_t buf_len = 0;
    ssize_t buf_pos = 0;

    while (socket_open || buf_pos < buf_len) {
        
        while (buf_pos < buf_len && !is_paused) {
            char c = buf[buf_pos++];
            
            if (!headers_skipped) {
                const char target[] = "\r\n\r\n";
                if (c == target[match_count]) {
                    match_count++;
                    if (match_count == 4) headers_skipped = 1;
                } else {
                    match_count = (c == '\r') ? 1 : 0;
                }
                continue;
            }

            putchar(c);
            if (c == '\n') {
                lines_printed++;
                if (lines_printed >= MAX_LINES) {
                    is_paused = 1;
                    printf("\033[7m--- Press space to scroll down ('q' to quit) ---\033[0m");
                    fflush(stdout);
                }
            }
        }
        fflush(stdout);

        
        if (is_paused || (buf_pos == buf_len && socket_open)) {
            fd_set readfds;
            FD_ZERO(&readfds);
            
            FD_SET(STDIN_FILENO, &readfds);
            int max_fd = STDIN_FILENO;
            
            if (socket_open && !is_paused && buf_pos == buf_len) {
                FD_SET(sockfd, &readfds);
                if (sockfd > max_fd) max_fd = sockfd;
            }

            if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
                perror("select");
                break;
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char c;
                if (read(STDIN_FILENO, &c, 1) > 0) {
                    if (c == 'q') {
                        socket_open = 0;
                        break;
                    } else if (c == ' ' && is_paused) {
                        is_paused = 0;
                        lines_printed = 0;
                        printf("\r\033[2K"); 
                        fflush(stdout);
                    }
                }
            }

            if (socket_open && !is_paused && buf_pos == buf_len && FD_ISSET(sockfd, &readfds)) {
                buf_len = read(sockfd, buf, BUF_SIZE);
                if (buf_len > 0) {
                    buf_pos = 0;
                } else if (buf_len == 0) {
                    socket_open = 0;
                } else {
                    perror("read socket");
                    break;
                }
            }
        }
    }

    if (shutdown(sockfd, SHUT_RDWR) != 0) {
        perror("shutdown");
        return 1;
    }
    close(sockfd);

    printf("\n"); 
    return 0;
}