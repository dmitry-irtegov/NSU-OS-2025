#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>

#define LINES_PER_SCREEN 25
#define BUF_CHUNK 4096

static struct termios orig_termios;

void set_raw_mode(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void restore_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void parse_url(const char *url, char *host, size_t host_sz, char *path, size_t path_sz) {
    const char *p = strstr(url, "://");
    p = p ? p + 3 : url;
    const char *slash = strchr(p, '/');
    if (slash) {
        size_t hlen = slash - p;
        if (hlen >= host_sz)
            hlen = host_sz - 1;
        memcpy(host, p, hlen);
        host[hlen] = '\0';
        strncpy(path, slash, path_sz - 1);
        path[path_sz - 1] = '\0';
    } else {
        strncpy(host, p, host_sz - 1);
        host[host_sz - 1] = '\0';
        strcpy(path, "/");
    }
}

int connect_to_host(const char *host, int port) {
    struct hostent *he = gethostbyname(host);
    if (!he) {
        fprintf(stderr, "Cannot resolve host: %s\n", host);
        exit(1);
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    memcpy(&sa.sin_addr, he->h_addr, he->h_length);
    sa.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("connect");
        close(fd);
        exit(1);
    }

    return fd;
}

char *find_header_end(char *buf, size_t len) {
    size_t i;
    for (i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' && buf[i + 3] == '\n') {
            return buf + i + 4;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return 1;
    }

    char host[256], path[2048];
    parse_url(argv[1], host, sizeof(host), path, sizeof(path));

    int sockfd = connect_to_host(host, 8001);

    char request[4096];
    int reqlen = snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);

    if (write(sockfd, request, reqlen) < 0) {
        perror("write");
        close(sockfd);
        return 1;
    }

    set_raw_mode();

    char *data = NULL; // данные из сети
    size_t data_cap = 0;
    size_t data_len = 0; //сколько байт в буфере
    size_t out_pos = 0; // до куда уже напечатали
    int headers_done = 0; // нашли ли конец заголовка
    int lines_printed = 0;
    int paused = 0;
    int sock_eof = 0; // закрыл ли сервер соединение

    int maxfd = (sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO) + 1;

    while (1) {
        if (headers_done && !paused) { // заголовки пропущены и нет паузы
            while (out_pos < data_len) {
                putchar(data[out_pos]);
                if (data[out_pos] == '\n') {
                    lines_printed++;
                    out_pos++;
                    if (lines_printed >= LINES_PER_SCREEN) {
                        fflush(stdout);
                        fprintf(stderr, "Press space to scroll down...");
                        paused = 1;
                        break;
                    }
                } else {
                    out_pos++;
                }
            }
            fflush(stdout);
        }

        if (sock_eof && (!headers_done || out_pos >= data_len)) // всё выведено
            break;

        fd_set rfds;
        FD_ZERO(&rfds);
        if (!sock_eof) // если открыт
            FD_SET(sockfd, &rfds);
        if (paused)
            FD_SET(STDIN_FILENO, &rfds);

        if (select(maxfd, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (!sock_eof && FD_ISSET(sockfd, &rfds)) {
            if (data_len + BUF_CHUNK > data_cap) {
                data_cap = data_cap ? data_cap * 2 : BUF_CHUNK * 4;
                data = realloc(data, data_cap);
                if (!data) {
                    perror("realloc");
                    break;
                }
            }
            ssize_t n = read(sockfd, data + data_len, BUF_CHUNK);
            if (n <= 0) {
                sock_eof = 1;
            } else {
                data_len += n;
            }
        }

        if (paused && FD_ISSET(STDIN_FILENO, &rfds)) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) > 0 && ch == ' ') {
                paused = 0;
                lines_printed = 0;
                fprintf(stderr, "\r\033[K");
            }
        }

        if (!headers_done) {
            char *body = find_header_end(data, data_len);
            if (body) {
                out_pos = body - data;
                headers_done = 1;
            }
        }
    }

    free(data);
    restore_mode();

    if (shutdown(sockfd, SHUT_RDWR) < 0) {
        perror("shutdown");
        return 1;
    }
    close(sockfd);
    return 0;
}
