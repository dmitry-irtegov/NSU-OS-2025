#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>
#include <stdbool.h>

#define LINES 25

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

int parse_http_url(const char* url, char* host, char* port, char* path) {
    if (!url || !host || !port || !path) {
        return -1;
    }
    
    strcpy(port, "80");
    strcpy(path, "/");
    host[0] = '\0';
    
    const char* current = url;
    if (strncmp(current, "http://", 7) == 0) {
        current += 7;
    }
    
    const char* path_start = strchr(current, '/');
    if (path_start != NULL) {
        if (strlen(path_start) >= 1024) return -1;
        strcpy(path, path_start);
    } else {
        path_start = current + strlen(current);
    }
    
    const char* host_start = current;
    const char* host_end = path_start;
    
    const char* colon_pos = NULL;
    for (const char* p = host_start; p < host_end; p++) {
        if (*p == ':') {
            colon_pos = p;
            break;
        }
    }
    
    if (colon_pos != NULL) {
        int host_len = colon_pos - host_start;
        if (host_len == 0 || host_len >= 256) return -1;
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';
        
        int port_len = host_end - colon_pos - 1;
        if (port_len >= 6) return -1;
        strncpy(port, colon_pos + 1, port_len);
        port[port_len] = '\0';
    } else {
        int host_len = host_end - host_start;
        if (host_len == 0 || host_len >= 256) return -1;
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';
    }
    
    return 0;
}

int connect_to_server(const char* host, const char* port) {
    struct addrinfo sock_criteria, *res; 
    int sock = -1; 
    memset(&sock_criteria, 0, sizeof(sock_criteria));

    sock_criteria.ai_family = AF_INET; 
    sock_criteria.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &sock_criteria, &res) != 0) {
        return -1;
    } 

    struct addrinfo *current = res;
    while (current != NULL) {
        sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        
        if (sock != -1) {
            if (connect(sock, current->ai_addr, current->ai_addrlen) != -1) {
                break;
            }
            close(sock);
        }
        current = current->ai_next;
    }

    freeaddrinfo(res);
    if (current == NULL) {
        return -1;
    }

    return sock;
}

void fetch_and_display(int sock, const char* host, const char* path) {
    char request[2048];
    int line_count = 0; 
    int reading_content = 0;

    bool paused = false;

    snprintf(request, sizeof(request), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    if (write(sock, request, strlen(request)) < 0) return;

    fd_set descriptor_set;
    int max_descriptor = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;
    char buffer_read[1024];
    int buf_len = 0;
    int buf_pos = 0;
    while (1) {
        FD_ZERO(&descriptor_set);
        FD_SET(STDIN_FILENO, &descriptor_set); 
        
        if (!paused && buf_pos == buf_len) {
            FD_SET(sock, &descriptor_set);   
        }

        if (paused || buf_pos == buf_len){
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
        }
        
        if (!paused) {
            if (buf_pos == buf_len && FD_ISSET(sock, &descriptor_set)) {
                buf_len = recv(sock, buffer_read, sizeof(buffer_read), 0); 
                if (buf_len <= 0) break;
                buf_pos = 0; 
            }

            int marker_index = 0;

            while(buf_pos < buf_len){
                char curr_byte = buffer_read[buf_pos++];

                if (!reading_content) {
                    if (curr_byte == '\r' || curr_byte == '\n') {
                        if (marker_index == 0 && curr_byte == '\r') {
                            marker_index = 1;
                        } else if (marker_index == 1 && curr_byte == '\n') {
                            marker_index = 2;
                        } else if (marker_index == 2 && curr_byte == '\r') {
                            marker_index = 3;
                        } else if (marker_index == 3 && curr_byte == '\n') {
                            reading_content = 1;
                            marker_index = 0;
                            continue;
                        } else {
                            marker_index = 0;
                        }
                    } else {
                        marker_index = 0;
                    }
                    continue;
                }
                putchar(curr_byte);

                if (curr_byte == '\n') {
                    line_count++;
                    if (line_count >= LINES) {
                        printf("--- Press space to continue ---\n");
                        fflush(stdout);
                        paused = true; 
                        break;
                    }
                }
            }
        }
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    char host[256];
    char port[16];
    char path[1024];
    int sock;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return 1;
    }
    
    if (parse_http_url(argv[1], host, port, path) < 0) {
        fprintf(stderr, "Invalid URL: %s\n", argv[1]);
        return 1;
    }
    
    if (tcgetattr(STDIN_FILENO, &orig_tty_settings) != 0) {
        perror("tcgetattr");
        return 1;
    }
    setup_unbuffered_mode();
    
    sock = connect_to_server(host, port);
    if (sock < 0) {
        fprintf(stderr, "Connection failed: %s:%s\n", host, port);
        return 1;
    }
    
    fetch_and_display(sock, host, path);
    
    shutdown(sock, SHUT_RDWR);
    close(sock);
    
    return 0;
}