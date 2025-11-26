#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/upper_socket"
#define BUF_SIZE 1024

int main(void) {
    int listen_fd = -1, conn_fd = -1;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    ssize_t n;

    if ((listen_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (unlink(SOCKET_PATH) == -1 && errno != ENOENT) {
        perror("unlink old socket");
        if (close(listen_fd) == -1) {
            perror("close listen_fd");
        }
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        if (close(listen_fd) == -1) {
            perror("close listen_fd");
        }
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 1) == -1) {
        perror("listen");
        if (close(listen_fd) == -1) {
            perror("close listen_fd");
        }
        if (unlink(SOCKET_PATH) == -1) {
            perror("unlink socket");
        }
        exit(EXIT_FAILURE);
    }

    if ((conn_fd = accept(listen_fd, NULL, NULL)) == -1) {
        perror("accept");
        if (close(listen_fd) == -1) {
            perror("close listen_fd");
        }
        if (unlink(SOCKET_PATH) == -1) {
            perror("unlink socket");
        }
        exit(EXIT_FAILURE);
    }

    while ((n = read(conn_fd, buf, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            buf[i] = (char)toupper((unsigned char)buf[i]);
        }

        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, n - written);
            if (w < 0) {
                perror("write stdout");
                close(conn_fd);
                close(listen_fd);
                unlink(SOCKET_PATH);
                exit(EXIT_FAILURE);
            }
            written += w;
        }
    }

    if (n < 0) {
        perror("read");
    }

    if (close(conn_fd) == -1) {
        perror("close conn_fd");
    }
    if (close(listen_fd) == -1) {
        perror("close listen_fd");
    }
    if (unlink(SOCKET_PATH) == -1) {
        perror("unlink socket");
    }

    return 0;
}
