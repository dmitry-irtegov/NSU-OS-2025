#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>
#include <stdbool.h>

#define LINES_PER_PAGE 25

struct termios orig_tty_settings;

void restore_terminal_state() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_tty_settings);
}

void setup_unbuffered_mode() {
    tcgetattr(STDIN_FILENO, &orig_tty_settings);
    atexit(restore_terminal_state);

    struct termios raw_config = orig_tty_settings;
    raw_config.c_lflag &= ~(ICANON | ECHO);
    raw_config.c_cc[VMIN] = 1;
    raw_config.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw_config);
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
    struct addrinfo sock_criteria, *res, *p;
    int sock = -1;

    memset(&sock_criteria, 0, sizeof(sock_criteria));
    sock_criteria.ai_family = AF_INET; 
    sock_criteria.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &sock_criteria, &res) != 0) return -1;

    for (p = res; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == -1) continue;
        
        if (connect(sock, p->ai_addr, p->ai_addrlen) != -1) break;
        close(sock);
    }

    freeaddrinfo(res);
    return (p == NULL) ? -1 : sock;
}

void fetch_and_display(int sock, const char* host, const char* path) {
    char request[2048];
    int line_count = 0; 
    int reading_content = 0;
    int state = 0; 
    bool paused = false;

    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    if (write(sock, request, strlen(request)) < 0) return;

    fd_set descriptor_set;
    int max_descriptor = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

    while (1) {
        FD_ZERO(&descriptor_set);
        FD_SET(STDIN_FILENO, &descriptor_set);
        
        if (!paused) {
            FD_SET(sock, &descriptor_set);
        }

        int status = select(max_descriptor + 1, &descriptor_set, NULL, NULL, NULL);
        if (status < 0) break;

        if (FD_ISSET(STDIN_FILENO, &descriptor_set)) {
            char key;
            if (read(STDIN_FILENO, &key, 1) > 0) {
                if (paused && key == ' ') {
                    paused = false;
                    line_count = 0;
                    fflush(stdout);
                }
            }
        }

        if (!paused && FD_ISSET(sock, &descriptor_set)) {
            char curr_byte;
            int bytes_received = recv(sock, &curr_byte, 1, 0);
            if (bytes_received <= 0) break;

            if (!reading_content) {
                if ((curr_byte == '\r' && (state == 0 || state == 2)) || (curr_byte == '\n' && (state == 1 || state == 3))) state++;
                else state = (curr_byte == '\r') ? 1 : 0;
                if (state == 4) reading_content = 1;
                continue;
            }

            putchar(curr_byte);
            fflush(stdout);

            if (curr_byte == '\n') {
                line_count++;
                if (line_count >= LINES_PER_PAGE) {
                    printf("[Press SPACE to scroll down]");
                    fflush(stdout);
                    paused = true;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Syntax: %s <URL> (must start with http://)\n", argv[0]);
        return EXIT_FAILURE;
    }

    char host[256], port[16], path[1024];

    if (!parse_address(argv[1], host, port, path)) {
        printf("Syntax: %s <URL> (must start with http://)\n", argv[0]);
        return EXIT_FAILURE;
    }

    setup_unbuffered_mode();

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