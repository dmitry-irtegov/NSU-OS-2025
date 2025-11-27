#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/demo_socket"
#define BUFF_SIZE 256

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFF_SIZE];
    ssize_t bytes_read;

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("create socet failed");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0,  sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s...\n", SOCKET_PATH);

    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) { 
        perror("accepted failed");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = read(client_fd, buffer, BUFF_SIZE)) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }
        if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
            perror("Write to stdout failed");
            break;
        }
    };

    if (bytes_read == -1) {
        perror("read failed");
    }

    printf("\nClient disconnected. Terminating server.\n");
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}