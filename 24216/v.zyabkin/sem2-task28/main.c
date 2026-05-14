#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <netdb.h>
#include <string.h>

#define BUFFER_SIZE 4096
#define MAX_LINES 25

// --- Блок работы с терминалом ---

void set_conio_terminal_mode(struct termios *orig) {
    struct termios new_t;

    if (tcgetattr(STDIN_FILENO, orig) < 0) {
        return;
    }

    new_t = *orig;
    new_t.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_t.c_cc[VMIN] = 1;
    new_t.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_t);
}

void reset_terminal_mode(struct termios *orig) {
    tcsetattr(STDIN_FILENO, TCSANOW, orig);
}

// --- Блок сетевой логики ---

int establish_connection(const char *host) {
    struct addrinfo hints = {0}, *res, *rpq;
    int sfd = -1;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "80", &hints, &res) != 0) {
        return -1;
    }

    for (rpq = res; rpq != NULL; rpq = rpq->ai_next) {
        sfd = socket(rpq->ai_family, rpq->ai_socktype, rpq->ai_protocol);

        if (sfd == -1) {
            continue;
        }

        if (connect(sfd, rpq->ai_addr, rpq->ai_addrlen) != -1) {
            break;
        }

        close(sfd);

        sfd = -1;
    }

    freeaddrinfo(res);

    return sfd;
}

// --- Блок обработки ввода ---

int wait_for_user_input() {
    char c;
    int found_space = 0;

    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == ' ') {
            found_space = 1;
        }
        
        fd_set fds;
        struct timeval tv = {0, 0};

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) {
            break;
        }
    }
    return found_space;
}

// --- Основная логика вывода ---

void process_and_print_data(int sockfd) {
    size_t data_size = 0, data_capacity = 0, position = 0;
    int lines = 0, paused = 0, check_head = 0, eof = 0;

    char *data = NULL;

    fd_set rset;

    while (!eof || position < data_size) {
        FD_ZERO(&rset);
        int mfd = -1;

        if (!eof) {
            FD_SET(sockfd, &rset);
            mfd = sockfd;
        }

        if (paused) {
            FD_SET(STDIN_FILENO, &rset);
            if(STDIN_FILENO > mfd) mfd = STDIN_FILENO;
        }

        struct timeval tv = {0, 10000};
        struct timeval *ptr = (!paused && position < data_size) ? &tv : NULL;

        if (select(mfd + 1, &rset, NULL, NULL, ptr) < 0) {
            break;
        }

        if (paused && FD_ISSET(STDIN_FILENO, &rset)) {
            if (wait_for_user_input()) {
                paused = 0;
                lines = 0;
                printf("\r                                        \r");
                fflush(stdout);
            }
        }

        if (!eof && FD_ISSET(sockfd, &rset)) {
            if (data_size + BUFFER_SIZE >= data_capacity) {
                data_capacity = data_capacity == 0 ? BUFFER_SIZE * 2 : data_capacity * 2;
                data = realloc(data, data_capacity);
            }
            int n = recv(sockfd, data + data_size, BUFFER_SIZE, 0);
            if (n <= 0) {
                eof = 1;
            } else {
                data_size += n;
            }
        }

        if (!paused && position < data_size) {
            if (!check_head) {
                char *p = strstr(data, "\r\n\r\n");
                if (p) {
                    check_head = 1;
                    position = (p - data) + 4;
                }
                else if (eof) {
                    check_head = 1;
                }
            }

            if (check_head) {
                while (position < data_size && !paused) {
                    putchar(data[position]);

                    if (data[position++] == '\n') {
                        lines++;
                    }

                    if (lines >= MAX_LINES) {
                        printf("\n--- Press space to scroll down ---");
                        fflush(stdout);
                        paused = 1;
                    }
                }
            }
        }
    }

    free(data);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strncmp(argv[1], "http://", 7) != 0) {
        return 1;
    }

    char host[256] = {0}, path[256] = {0};
    if (sscanf(argv[1] + 7, "%255[^/]%255s", host, path) < 1) {
        return 1;
    }

    if (path[0] == '\0') {
        strcpy(path, "/");
    }

    int sockfd = establish_connection(host);
    if (sockfd < 0) {
        return 1;
    }

    char req[1024];
    snprintf(req, sizeof(req), "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    send(sockfd, req, strlen(req), 0);

    struct termios old_t;
    set_conio_terminal_mode(&old_t);
    
    process_and_print_data(sockfd);

    reset_terminal_mode(&old_t);
    
    close(sockfd);

    return 0;
}