#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>

#define CHUNK_SIZE 4096
#define LINES_PER_PAGE 25

struct termios orig_term_settings;

void reset_console() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term_settings);
}

void init_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_term_settings);
    atexit(reset_console);

    struct termios raw_mod = orig_term_settings;
    raw_mod.c_lflag &= ~(ICANON | ECHO);
    raw_mod.c_cc[VMIN] = 1;
    raw_mod.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw_mod);
}

int parse_address(const char* url, char* host, char* port, char* path) {
    if (strstr(url, "http://") != url) return 0;

    const char* base_url = url + 7;
    const char* slash_pos = strchr(base_url, '/');
    char temp_host[1024] = {0};

    if (slash_pos) {
        size_t host_len = slash_pos - base_url;
        if (host_len >= sizeof(temp_host)) return 0;
        strncpy(temp_host, base_url, host_len);

        size_t path_len = strlen(slash_pos);
        if (path_len >= 1024) return 0;
        strcpy(path, slash_pos);
    } else {
        size_t host_len = strlen(base_url);
        if (host_len >= sizeof(temp_host)) return 0;
        strcpy(temp_host, base_url);
        strcpy(path, "/");
    }

    char* colon_pos = strchr(temp_host, ':');
    if (colon_pos) {
        *colon_pos = '\0';
        if (strlen(temp_host) >= 256) return 0;
        strcpy(host, temp_host);
        if (strlen(colon_pos + 1) >= 16) return 0;
        strcpy(port, colon_pos + 1);
    } else {
        if (strlen(temp_host) >= 256) return 0;
        strcpy(host, temp_host);
        strcpy(port, "80");
    }

    return 1;
}

int connect_to_server(const char* host, const char* port) {
    struct addrinfo hints, *res, *p;
    int sock_fd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) return -1;

    for (p = res; p != NULL; p = p->ai_next) {
        sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock_fd == -1) continue;
        
        if (connect(sock_fd, p->ai_addr, p->ai_addrlen) != -1) break;
        close(sock_fd);
    }

    freeaddrinfo(res);
    return (p == NULL) ? -1 : sock_fd;
}

void pause_for_user() {
    printf("\n[Press SPACE to continue...]\n");
    fflush(stdout);

    fd_set kbd_fd;
    char key;

    while (1) {
        FD_ZERO(&kbd_fd);
        FD_SET(STDIN_FILENO, &kbd_fd);
        
        if (select(STDIN_FILENO + 1, &kbd_fd, NULL, NULL, NULL) > 0) {
            if (read(STDIN_FILENO, &key, 1) > 0 && key == ' ') {
                break;
            }
        }
    }
}

void fetch_and_display(int sock, const char* host, const char* path) {
    char recv_buf[CHUNK_SIZE];
    char line_buf[CHUNK_SIZE * 2];
    int line_len = 0;
    char request[2048];
    int line_count = 0;
    int is_body = 0;

    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    if (write(sock, request, strlen(request)) < 0) return;

    fd_set net_fd;
    struct timeval tv;

    while (1) {
        FD_ZERO(&net_fd);
        FD_SET(sock, &net_fd);
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int status = select(sock + 1, &net_fd, NULL, NULL, &tv);

        if (status <= 0) break;

        if (FD_ISSET(sock, &net_fd)) {
            int bytes_received = recv(sock, recv_buf, sizeof(recv_buf), 0);
            if (bytes_received <= 0) break;

            for (int i = 0; i < bytes_received; i++) {
                char c = recv_buf[i];

                if (c == '\n') {
                    if (line_len > 0 && line_buf[line_len - 1] == '\r') line_len--;
                    line_buf[line_len] = '\0';

                    if (!is_body) {
                        if (line_len == 0) is_body = 1;
                    } else {
                        printf("%s\n", line_buf);
                        line_count++;
                        if (line_count >= LINES_PER_PAGE) {
                            pause_for_user();
                            line_count = 0;
                        }
                    }
                    line_len = 0;
                } else {
                    if (line_len < (int)sizeof(line_buf) - 1)
                        line_buf[line_len++] = c;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Syntax: %s <URL> start with http://\n", argv[0]);
        return EXIT_FAILURE;
    }

    char host[256], port[16], path[1024];

    if (!parse_address(argv[1], host, port, path)) {
        printf("Syntax: %s <URL> start with http://\n", argv[0]);
        return EXIT_FAILURE;
    }

    init_raw_mode();

    int sock = connect_to_server(host, port);
    if (sock < 0) {
        printf("Error: could not connect to %s:%s\n", host, port);
        return EXIT_FAILURE;
    }

    fetch_and_display(sock, host, path);

    shutdown(sock, SHUT_RDWR);
    close(sock);

    return EXIT_SUCCESS;
}