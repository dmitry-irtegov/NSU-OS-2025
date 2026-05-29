#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>

struct termios orig_termios;

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void configure_terminal() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    atexit(restore_terminal);

    struct termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void extract_url_components(const char *url, char *host, char *port, char *path) {
    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }

    const char *path_start = strchr(url, '/');
    const char *port_start = strchr(url, ':');

    if (path_start) {
        strcpy(path, path_start);
    } else {
        strcpy(path, "/");
    }

    int host_len;
    if (port_start && (!path_start || port_start < path_start)) {
        host_len = port_start - url;
        int port_len = path_start ? (path_start - port_start - 1) : strlen(port_start + 1);
        strncpy(port, port_start + 1, port_len);
        port[port_len] = '\0';
    } else {
        host_len = path_start ? (path_start - url) : strlen(url);
        strcpy(port, "80");
    }
    strncpy(host, url, host_len);
    host[host_len] = '\0';
}

int create_connection(const char *host, const char *port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo error");
        return -1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket error");
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect error");
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}

void send_http_get(int sock, const char *host, const char *path) {
    char request[1024];
    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    if (send(sock, request, strlen(request), 0) < 0) {
        perror("send error");
    }
}

void process_response(int sockfd) {
    char *buffer = NULL;
    size_t buf_len = 0;
    size_t buf_cap = 0;

    int is_connected = 1;
    int headers_skipped = 0;
    int lines_printed = 0;
    int wait_for_space = 0;

    while (is_connected || buf_len > 0) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        
        if (is_connected) FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = (is_connected && sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select error");
            break;
        }

        if (is_connected && FD_ISSET(sockfd, &read_fds)) {
            char temp[4096];
            ssize_t bytes_recv = recv(sockfd, temp, sizeof(temp), 0);
            if (bytes_recv > 0) {
                if (buf_len + bytes_recv > buf_cap) {
                    buf_cap = (buf_cap == 0) ? 8192 : (buf_cap * 2) + bytes_recv;
                    buffer = realloc(buffer, buf_cap);
                }
                memcpy(buffer + buf_len, temp, bytes_recv);
                buf_len += bytes_recv;
            } else if (bytes_recv == 0) {
                is_connected = 0; 
            } else {
                perror("recv error");
                is_connected = 0;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char key;
            if (read(STDIN_FILENO, &key, 1) > 0) {
                if (wait_for_space && key == ' ') {
                    wait_for_space = 0;
                    lines_printed = 0;
                    printf("\r\033[K"); 
                    fflush(stdout);
                }
            }
        }

        while (!wait_for_space && buf_len > 0) {
            if (!headers_skipped) {
                char *hdr_end = strstr(buffer, "\r\n\r\n");
                if (hdr_end) {
                    headers_skipped = 1;
                    size_t hdr_size = (hdr_end - buffer) + 4;
                    memmove(buffer, buffer + hdr_size, buf_len - hdr_size);
                    buf_len -= hdr_size;
                } else {
                    break; 
                }
            } else {
                size_t i = 0;
                while (i < buf_len && lines_printed < 25) {
                    if (buffer[i] == '\n') {
                        lines_printed++;
                    }
                    i++;
                }

                fwrite(buffer, 1, i, stdout);
                fflush(stdout);

                memmove(buffer, buffer + i, buf_len - i);
                buf_len -= i;

                if (lines_printed >= 25) {
                    wait_for_space = 1;
                    printf("\n--- Press space to scroll down ---");
                    fflush(stdout);
                }
            }
        }
    }
    
    if (buffer) free(buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <HTTP URL>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char host[256];
    char port[16];
    char path[512];

    extract_url_components(argv[1], host, port, path);

    configure_terminal();
    setbuf(stdin, NULL);

    int sockfd = create_connection(host, port);
    if (sockfd < 0) {
        return EXIT_FAILURE;
    }

    send_http_get(sockfd, host, path);
    process_response(sockfd);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    return EXIT_SUCCESS;
}