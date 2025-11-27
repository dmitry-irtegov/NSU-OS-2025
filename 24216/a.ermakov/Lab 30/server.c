#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/uds_socket"
#define BUF_SIZE 256

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    unlink(SOCKET_PATH);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    printf("Server is waiting for a client...\n");

    if ((client_fd = accept(server_fd, NULL, NULL)) < 0) {
        perror("accept");
        close(server_fd);
        exit(1);
    }

    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = read(client_fd, buf, BUF_SIZE)) > 0) {
        for (int i = 0; i < n; i++)
            buf[i] = toupper((unsigned char)buf[i]);
        write(STDOUT_FILENO, buf, n);
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}
