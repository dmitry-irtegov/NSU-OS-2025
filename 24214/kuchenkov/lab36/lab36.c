#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <termios.h>

/*
    36. Псевдомногопоточный HTTP-клиент
+    Реализуйте простой HTTP-клиент. Он принимает один параметр командной строки –
+    URL. Требуется поддержка протокола HTTP/1.0 без поддержки TLS (https). Клиент
    делает запрос по указанному URL и выдает тело ответа на терминал как текст (т.е.
+    если в ответе HTML, то распечатывает его исходный текст без форматирования).
+    Вывод производится по мере того, как данные поступают из HTTP-соединения. Когда
    будет выведено более экрана (более 25 строк) данных, клиент должен продолжить
    прием данных, но должен остановить вывод и выдать приглашение Press space to
    scroll down.
    При нажатии пользователем клиент должен вывести следующий экран данных. Для
    одновременного считывания данных с терминала и из сетевого соединения
    используйте системный вызов select или poll.
+    При реализации этой задачи запрещено использовать libcurl, boost.http/boost.asio и
+    golang net/http. Доступ к серверу должен осуществляться через сырой сокет TCP. Я в
+    курсе, что это делает практически неосуществимой поддержку TLS, HTTP/2.x и
+    значительной части протокола HTTP/1.1, но условиями задачи это не предполагается.
*/

struct termios termios_orig;

int connect_to_host(const char *url) {
    char host[256];
    char path[1024];
    char* host_start;
    char* path_start;

    if (strstr(url, "http://") == url) {
        host_start = url + strlen("http://");
    } else if (strstr(url, "https://") == url) {
        fprintf(stderr, "https is not supported\n");
        return -1;
    } else {
        host_start = url;
    }

    path_start = strchr(host_start, '/');
    if (path_start != NULL) {
        int host_len = path_start - host_start;
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';
        strcpy(path, path_start);
    } else {
        strcpy(host, host_start);
        strcpy(path, "/");
    }

    printf("Host: %s\nPath: %s\n", host, path);

    struct hostent *hostent = gethostbyname(host);
    if (hostent == NULL) {
        fprintf(stderr, "Failed to resolve host: %s\n", host);
        return -1;
    }
    
    printf("Hostent: %p\n", hostent->h_addr_list[0]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    memcpy(&server_addr.sin_addr, hostent->h_addr_list[0], hostent->h_length);

    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        return -1;
    }

    printf("Successfully connected to host.\n");

    char request[2048];
    sprintf(request, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    if (send(client_socket, request, strlen(request), 0) == -1) {
        perror("send");
        return -1;
    }

    printf("Successfully sent request.\n");
    return client_socket;
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
        perror("read in skip_http_headers");
    } else { // bytes_read == 0
        fprintf(stderr, "Connection closed before end of headers\n");
    }
    return -1;
}

void process_response(int client_socket) {
    if (skip_http_headers(client_socket) == -1) {
        return;
    }
    
    fd_set set;
    char buffer[4096];
    int line_count = 0;
    int paused = 0;
    int bytes_in_buffer = 0;
    int buffer_pos = 0;
    
    int running = 1;
    int connection_closed = 0;

    while (running) {
        if (buffer_pos > 0) {
            int remaining_data = bytes_in_buffer - buffer_pos;
            if (remaining_data > 0) {
                memmove(buffer, &buffer[buffer_pos], remaining_data);
            }
            bytes_in_buffer = remaining_data;
            buffer_pos = 0;
        }

        FD_ZERO(&set);
        int space_available = sizeof(buffer) - bytes_in_buffer;
        if (!connection_closed && space_available > 0) {
            FD_SET(client_socket, &set); 
        }
        FD_SET(STDIN_FILENO, &set);

        struct timeval timeout = {0, 0};
        int ready = select(client_socket + 1, &set, NULL, NULL, (buffer_pos < bytes_in_buffer) ? &timeout : NULL);

        if (ready == -1) {
            perror("select");
            running = 0;
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &set)) {
            char c;
            int read_ans = read(STDIN_FILENO, &c, 1);
            if (read_ans == 1) {
                if (c == ' ' && paused) {
                    line_count = 0;
                    paused = 0;
                }
            } else if (read_ans == 0) {
                printf("EOF on stdin\n");
                break;
            } else {
                perror("read");
                break;
            }
        }

        if (FD_ISSET(client_socket, &set)) {
            int bytes_read = recv(client_socket, &buffer[bytes_in_buffer], space_available, 0);
            if (bytes_read <= 0) {
                connection_closed = 1;
            } else {
                bytes_in_buffer += bytes_read;
            }
        }

        if (!paused) {
            while (buffer_pos < bytes_in_buffer) {
                if (write(STDOUT_FILENO, &buffer[buffer_pos], 1) == -1) {
                    perror("write");
                    running = 0;
                    break;
                }
                if (buffer[buffer_pos] == '\n') {
                    line_count++;
                }
                buffer_pos++;
                if (line_count >= 25) {
                    paused = 1;
                    printf("\n>> Press space to scroll down <<\n");
                    break;
                }
            }
        }
        
        if (connection_closed && buffer_pos == bytes_in_buffer) {
            printf("\nConnection closed.\n");
            running = 0;
        }
    }
}

void backup_termios() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &termios_orig) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void change_termios() {
    if (tcgetattr(STDIN_FILENO, &termios_orig) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    atexit(backup_termios);
    struct termios termios_new = termios_orig;
    termios_new.c_lflag &= ~(ICANON | ECHO);
    termios_new.c_cc[VMIN] = 1;
    termios_new.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &termios_new) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return 1;
    }

    int client_socket = connect_to_host(argv[1]);
    if (client_socket == -1) {
        return 1;
    }

    change_termios();

    process_response(client_socket);

    close(client_socket);
    return 0;
}