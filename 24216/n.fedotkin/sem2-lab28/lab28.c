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
    char temp_host[512] = {0};

    if (slash_pos) {
        strncpy(temp_host, base_url, slash_pos - base_url);
        strcpy(path, slash_pos);
    } else {
        strcpy(temp_host, base_url);
        strcpy(path, "/");
    }

    char* colon_pos = strchr(temp_host, ':');
    if (colon_pos) {
        *colon_pos = '\0';
        strcpy(host, temp_host);
        strcpy(port, colon_pos + 1);
    } else {
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
    char buffer[CHUNK_SIZE + 1];
    char request[2048];
    int line_count = 0;
    int is_body = 0;

    sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
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
            int bytes_received = recv(sock, buffer, CHUNK_SIZE, 0);
            if (bytes_received <= 0) break;
            
            buffer[bytes_received] = '\0';
            char *ptr = buffer;

            while (ptr && *ptr) {
                char *newline = strchr(ptr, '\n');
                if (newline) *newline = '\0';

                size_t len = strlen(ptr);
                if (len > 0 && ptr[len - 1] == '\r') ptr[len - 1] = '\0';
                len = strlen(ptr);

                if (!is_body) {
                    if (len == 0) is_body = 1;
                } else {
                    printf("%s\n", ptr);
                    line_count++;
                    
                    if (line_count >= LINES_PER_PAGE) {
                        pause_for_user();
                        line_count = 0;
                    }
                }

                if (newline) ptr = newline + 1;
                else break;
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
    close(sock);

    return EXIT_SUCCESS;
}