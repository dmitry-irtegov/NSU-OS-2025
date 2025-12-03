#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/socket_lab30"
#define BUFFER_SIZE 1024 

int main(void) {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];

    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Error: socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;

    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: connect failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type text to send:\n");

    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t length = strlen(buffer);
        size_t total_written = 0;

        while (total_written < length) {
            ssize_t bytes_written = write(client_fd, buffer + total_written, length - total_written);
            
            if (bytes_written < 0) {
                perror("Error: write failed");
                close(client_fd);
                exit(EXIT_FAILURE);
            }
            total_written += bytes_written;
        }
    }

    close(client_fd);
    exit(EXIT_SUCCESS);
}