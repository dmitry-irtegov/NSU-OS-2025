#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/uds_lab30.sock"

int main(void) {
    int fd = -1;
    struct sockaddr_un addr;
    char buf[4096];

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    for (;;) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            close(fd);
            return 1;
        }
        if (n == 0) break;

        ssize_t sent = 0;
        while (sent < n) {
            ssize_t s = send(fd, buf + sent, (size_t)(n - sent), 0);
            if (s < 0) {
                if (errno == EINTR) continue;
                perror("send");
                close(fd);
                return 1;
            }
            sent += s;
        }
    }

    close(fd);
    return 0;
}
