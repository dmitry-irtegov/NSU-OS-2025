#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h> 

char* sock_path = "./sock";
int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

int main() {
    int server_fd, new_socket;
    fd_set readfds;
    fd_set tempfds;
    char buffer[BUFSIZ];

    struct sockaddr_un address;
    int addrlen = sizeof(address);
    int max_fd;
    int i, valread;

    signal(SIGINT, signal_handler);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, sock_path, sizeof(address.sun_path) - 1);

    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) { 
        perror("listen failed");
        close(server_fd);
        unlink(sock_path);
        exit(EXIT_FAILURE);
    }

    printf("The server is running. Waiting for connections...(Ctrl+C to finish)\n");

    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    max_fd = server_fd;

    while (keep_running) {
        tempfds = readfds;

        int activity = select(max_fd + 1, &tempfds, NULL, NULL, NULL);

        if (activity == -1) {
            if (errno == EINTR) continue;
            perror("select error");
            break;
        }

        if (FD_ISSET(server_fd, &tempfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                (socklen_t*)&addrlen)) < 0) {
                perror("accept error");
                continue;
            }
            printf("New connection, socket fd = %d\n", new_socket);

            FD_SET(new_socket, &readfds); 
            if (new_socket > max_fd) {
                max_fd = new_socket;
            }
        }

        for (i = server_fd + 1; i <= max_fd; i++) {
            if (FD_ISSET(i, &tempfds)) {
                valread = read(i, buffer, BUFSIZ - 1);
                if (valread == 0) {
                    printf("Client %d disconnected\n", i);
                    close(i);
                    FD_CLR(i, &readfds); 
                    
                    if (i == max_fd) {
                        while (max_fd > server_fd && !FD_ISSET(max_fd, &readfds)) {
                            max_fd--;
                        }
                    }
                } 
                else if (valread > 0) {
                    buffer[valread] = '\0';
                    for (int j = 0; j < valread; j++) {
                        buffer[j] = toupper(buffer[j]);
                    }
                    printf("Received from client %d: %s", i, buffer);
                    if (buffer[valread - 1] != '\n') {
                        printf("\n");
                    }
                }
                else {
                    perror("read error");
                }
            }
        }
    }

    for (i = server_fd + 1; i <= max_fd; i++) {
        if (FD_ISSET(i, &readfds)) {
            close(i);
        }
    }

    close(server_fd);
    unlink(sock_path);
    printf("\nStopping the server\n");
    return 0;
}
