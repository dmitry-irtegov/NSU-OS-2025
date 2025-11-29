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
#define MAX_CLIENTS 1024

fd_set main_set;
int listen_fd;
int active_fds[MAX_CLIENTS];
int active_count = 0;
int fd_max = 0;
char buffer[BUFFER_SIZE];

int setup_server() {
    int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_socket;
}

void checking_fd(fd_set *read_set) {
    for (int i = 0; i < active_count; i++) {
        int current_fd = active_fds[i];

        if (!FD_ISSET(current_fd, read_set)) {
            continue;
        }

        if (current_fd == listen_fd) {
            int new_socket = accept(listen_fd, NULL, NULL);
            if (new_socket == -1) {
                perror("accept");
            } else {
                FD_SET(new_socket, &main_set);
                if (new_socket > fd_max) {
                    fd_max = new_socket;
                }
                if (active_count < MAX_CLIENTS) {
                    active_fds[active_count++] = new_socket;
                    printf("New client connected (FD: %d)\n", new_socket);
                } else {
                    fprintf(stderr, "Too many clients\n");
                    close(new_socket);
                    FD_CLR(new_socket, &main_set);
                }
            }
            continue;
        } 
        
        int bytes_read = read(current_fd, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("Client disconnected (FD: %d)\n", current_fd);
            } else {
                perror("read");
            }
            close(current_fd);
            FD_CLR(current_fd, &main_set);

            for (int j = i; j < active_count - 1; j++) {
                active_fds[j] = active_fds[j + 1];
            }
            active_count--;
            i--;
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

int main() {
    listen_fd = setup_server();

    printf("Server listening on %s\n", SOCKET_PATH);

    FD_ZERO(&main_set);
    FD_SET(listen_fd, &main_set);

    fd_max = listen_fd;
    active_fds[0] = listen_fd;
    active_count = 1;

    while (1) {
        fd_set read_set = main_set;

        if (select(fd_max+1, &read_set, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        checking_fd(&read_set);
    }

    close(listen_fd);
    unlink(SOCKET_PATH);
    return 0;
}