#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

int main() {
    char socket_path[] = "/tmp/somesocket";

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        unlink(socket_path);
        exit(1);
    }

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        unlink(socket_path);
        exit(1);
    }

    unsigned char buff[BUFFER_SIZE];
    while (1) {
        ssize_t n = recv(client_fd, buff, BUFFER_SIZE, 0);
        if (n > 0) {
            for (int j = 0; buff[j] != '\0'; j++) {
                buff[j] = toupper(buff[j]);
            }
            write(STDOUT_FILENO, buff, n);
        } else if (n == 0) {
            break;
        } else {
            perror("recv");
            unlink(socket_path);
            exit(1);
        }
    }

    unlink(socket_path);

    return 0;
}
