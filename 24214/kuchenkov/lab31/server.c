#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/socket"
#define BUFFER_SIZE 1024

int main() {
    int client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    
    if (bind(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(client_socket, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s\n", SOCKET_PATH);
    
    fd_set set;
    fd_set read_set;    
    int fd_max = client_socket;
    int active_fds[1024];
    int active_count = 1;
    active_fds[0] = client_socket;

    FD_ZERO(&set);
    FD_ZERO(&read_set);
    FD_SET(client_socket, &set);

    char buffer[BUFFER_SIZE];

    while (1) {
        read_set = set;
        if (select(fd_max+1, &read_set, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        for (int idx = 0; idx < active_count; idx++) {
            int i = active_fds[idx];
            if (FD_ISSET(i, &read_set)) {
                if (i == client_socket) {
                    int new_socket = accept(client_socket, NULL, NULL);
                    if (new_socket == -1) {
                        perror("accept");
                    } else {
                        FD_SET(new_socket, &set);
                        if (new_socket > fd_max) {
                            fd_max = new_socket;
                        }
                        if (active_count < 1024) {
                            active_fds[active_count++] = new_socket;
                        }
                        printf("New client connected (FD: %d)\n", new_socket);
                    }
                } else {
                    int bytes_read = read(i, buffer, BUFFER_SIZE);
                    if (bytes_read <= 0) {
                        if (bytes_read == 0) {
                            printf("Client disconnected (FD: %d)\n", i);
                        } else {
                            perror("read");
                        }
                        close(i);
                        FD_CLR(i, &set);
                        for (int j = idx; j < active_count - 1; j++) {
                            active_fds[j] = active_fds[j + 1];
                        }
                        active_count--;
                        idx--;
                    } else {
                        for (int j = 0; j < bytes_read; j++) {
                            buffer[j] = toupper((unsigned char)buffer[j]);
                        }
                        if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
                            perror("write");
                        }
                    }
                }
            }
        }
    }

    close(client_socket);
    unlink(SOCKET_PATH);
    return 0;
}