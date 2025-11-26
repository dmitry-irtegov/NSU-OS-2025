#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/upper_socket"
#define BUF_SIZE 1024

int main(void) {
    int sock_fd;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    ssize_t n;

    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    while ((n = read(STDIN_FILENO, buf, BUF_SIZE)) > 0) {
        ssize_t sent = 0;
        while (sent < n) {
            ssize_t s = write(sock_fd, buf + sent, n - sent);
            if (s < 0) {
                perror("write to socket");
                close(sock_fd);
                exit(EXIT_FAILURE);
            }
            sent += s;
        }
    }

    if (n < 0) {
        perror("read stdin");
    }

    if (close(sock_fd) == -1) {
        perror("close socket");
        exit(EXIT_FAILURE);
    }
    return 0;
}
