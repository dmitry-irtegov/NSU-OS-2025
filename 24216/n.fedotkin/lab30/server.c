#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/socket_lab30"
#define BUFFER_SIZE 256

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    unlink(SOCKET_PATH);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Error: socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) < 0) {
        perror("Error: listen failed");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    printf("Server started.\n");

    if ((client_fd = accept(server_fd, NULL, NULL)) < 0) {
        perror("Error: accept failed");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }
        if (write(STDOUT_FILENO, buffer, bytes_read) < 0) {
            perror("Error: write to stdout failed");
            break;
        }
    }

    if (bytes_read < 0) {
        perror("Error: read failed");
    } else {
        printf("\nClient disconnected.\n");
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    exit(EXIT_SUCCESS);
}