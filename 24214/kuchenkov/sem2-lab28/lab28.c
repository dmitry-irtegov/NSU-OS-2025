/*
 * 28. Псевдомногопоточный HTTP-клиент
 *
 * Реализуйте простой HTTP-клиент. Он принимает один параметр командной строки – URL.
 * Клиент делает запрос по указанному URL и выдает тело ответа на терминал как текст
 * (т.е. если в ответе HTML, то распечатывает его исходный текст без форматирования).
 * Вывод производится по мере того, как данные поступают из HTTP-соединения.
 * Когда будет выведено более экрана (более 25 строк) данных, клиент должен продолжить прием данных,
 * но должен остановить вывод и выдать приглашение Press space to scroll down.
 * 
 * При нажатии пользователем клиент должен вывести следующий экран данных.
 * Для одновременного считывания данных с терминала и из сетевого соединения используйте системный вызов select.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define HOST_LEN 256
#define BUFFER_SIZE 1024
#define RESP_BUFFER_SIZE 4096

struct termios orig_termios;

int parse_url(char *url, char *host, int *port, char *path) {
    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "Unable to parse url\n");
        return -1;
    }

    char *p = url + 7;
    while (*p && *p != '/' && *p != ':') {
        p++;
    }
    int hostlen = p - (url + 7);
    if (hostlen == 0 || hostlen >= HOST_LEN) {
        fprintf(stderr, "Host name too long\n");
        return -1;
    }
    memcpy(host, url + 7, hostlen);
    host[hostlen] = '\0';

    if (*p == ':') {
        p++;
        *port = atoi(p);
        while (*p && *p != '/') {
            p++;
        }
    } else {
        *port = 80;
    }

    if (*p == '/') {
        strncpy(path, p, BUFFER_SIZE-1);
        path[BUFFER_SIZE-1] = '\0';
    } else {
        strncpy(path, "/", BUFFER_SIZE);
    }

    return 0;
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void sigint_handler(int sig) {
    (void)sig;
    restore_terminal();
    exit(EXIT_FAILURE);
}

void set_terminal() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    atexit(restore_terminal);
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    struct termios new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

int setup_server(char *host, int port, char* path) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "Unable to create socket\n");
        return -1;
    }

    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Unable to resolve host\n");
        close(socket_fd);
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "Unable to connect to server\n");
        close(socket_fd);
        return -1;
    }

    char request[BUFFER_SIZE];
    snprintf(request, BUFFER_SIZE, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);

    if (write(socket_fd, request, strlen(request)) < 0) {
        fprintf(stderr, "Unable to send request\n");
        return -1;
    }

    return socket_fd;
}


int skip_http_headers(int socket_fd) {
    char buffer[4] = {0};
    int bytes_read;
    while ((bytes_read = read(socket_fd, &buffer[3], 1)) > 0) {
        if (buffer[0] == '\r' && buffer[1] == '\n' &&
            buffer[2] == '\r' && buffer[3] == '\n') {
            return 0;
        }
        buffer[0] = buffer[1];
        buffer[1] = buffer[2];
        buffer[2] = buffer[3];
    }

    if (bytes_read == -1) {
        fprintf(stderr, "Unable to read in skip_http_headers");
    } else {
        fprintf(stderr, "Connection closed before end of headers\n");
    }
    return -1;
}

int read_response(int socket_fd) {
    set_terminal();

    if (skip_http_headers(socket_fd) == -1) {
        return -1;
    }

    fd_set read_fds;
    char buffer[RESP_BUFFER_SIZE];

    int bytes_in_buffer = 0;
    int buffer_pos = 0;
    int line_count = 0;
    int paused = 0;
    int connection_closed = 0;

    int maximum_fd = socket_fd > STDIN_FILENO ? socket_fd : STDIN_FILENO;

    while (1) {
        if (buffer_pos > 0) {
            int remaining_data = bytes_in_buffer - buffer_pos;
            if (remaining_data > 0) {
                memmove(buffer, buffer + buffer_pos, remaining_data);
            }
            bytes_in_buffer = remaining_data;
            buffer_pos = 0;
        }

        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int space_available = RESP_BUFFER_SIZE - bytes_in_buffer;

        if (!connection_closed && space_available > 0) {
            FD_SET(socket_fd, &read_fds);
        }

        if (select(maximum_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "Error in select\n");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                if (c == ' ' && paused) {
                    paused = 0;
                    line_count = 0;
                }
            }
        }

        if (!connection_closed && FD_ISSET(socket_fd, &read_fds)) {
            int bytes = read(socket_fd, buffer + bytes_in_buffer, space_available);
            if (bytes > 0) {
                bytes_in_buffer += bytes;
            } else if (bytes == 0) {
                connection_closed = 1;
            } else {
                fprintf(stderr, "Error reading from socket\n");
                break;
            }
        }

        if (!paused) {
             while (buffer_pos < bytes_in_buffer) {
                putchar(buffer[buffer_pos]);
                if (buffer[buffer_pos] == '\n') {
                    line_count++;
                }
                buffer_pos++;
                if (line_count >= 25) {
                    paused = 1;
                    printf("\n>> Press SPACE to scroll down <<\n");
                    fflush(stdout);
                    break;
                }
            }
        }

        if (connection_closed && buffer_pos >= bytes_in_buffer) {
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return 1;
    }

    char *url = argv[1];
    char host[HOST_LEN];
    int port;
    char path[BUFFER_SIZE];

    if (parse_url(url, host, &port, path) != 0) {
        fprintf(stderr, "Error: Invalid URL\n");
        return 1;
    }

    printf("Connecting to Host: %s, Port: %d, Path: %s\n", host, port, path);

    int socket_fd = setup_server(host, port, path);
    if (socket_fd == -1) {
        return 1;
    }

    printf("Connected! Waiting for response...\n\n");

    if (read_response(socket_fd) != 0) {
        close(socket_fd);
        return 1;
    }
    close(socket_fd);

    return 0;
}
