#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main() {
    int fd;
    struct sockaddr_un addr;
    char buff[BUF_SIZE];
    const char *socket_path = "/tmp/u_socket";

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, '\0', sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes;
    while ((bytes = read(STDIN_FILENO, buff, BUF_SIZE)) > 0) {
        if (write(fd, buff, bytes) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    if (bytes == -1) {
        perror("read fail");
        exit(EXIT_FAILURE);
    }

    close(fd);
    exit(EXIT_SUCCESS);
}

