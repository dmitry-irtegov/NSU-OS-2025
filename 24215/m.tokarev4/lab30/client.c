#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "./socket"
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    int bytes_written;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Enter text (Ctrl+D to finish):\n");

    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        bytes_written = write(sock_fd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("write");
            break;
        }
    }

    printf("Connection closed\n");

    close(sock_fd);

    return 0;
}
