#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#define BUFFER_SIZE 4096

int main() {
    int client_fd;
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        return -1;
    }

    char* socket_path = "./upp_socket";
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_path, sizeof(server_addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect error");
        close(client_fd);
        return -1;
    }

    printf("Connected. Enter text: ");

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        if (write(client_fd, buffer, strlen(buffer)) == -1) {
            perror("write error");
            break;
        }
    }

    printf("Client disconnected.\n");
    close(client_fd);
    return 0;
}
