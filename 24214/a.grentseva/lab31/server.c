#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

int main() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        perror("Error socket"); 
        exit(1); 
    }

    unlink(SOCKET_PATH);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { 
        perror("Error bind"); 
        exit(1); 
    }
    if (listen(server_fd, MAX_CLIENTS) < 0) { 
        perror("Error listen"); 
        exit(1); 
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] > 0) {
                FD_SET(clients[i], &readfds);
                if (clients[i] > max_fd) max_fd = clients[i];
            }
        }

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        if (FD_ISSET(server_fd, &readfds)) {
            int cfd = accept(server_fd, NULL, NULL);
            if (cfd >= 0) {
                int added = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] < 0) { 
                        clients[i] = cfd; 
                        added = 1; 
                        break; 
                    }
                }
                if (!added) { 
                    printf("Too many clients\n"); 
                    close(cfd); 
                }
                else printf("Client connected\n");
            }
        }

        char buf[BUFFER_SIZE];
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                ssize_t n = read(sd, buf, BUFFER_SIZE-1);
                if (n > 0) {
                    buf[n] = '\0';
                    for (int j = 0; j < n; j++) buf[j] = toupper((unsigned char)buf[j]);
                    printf("%s", buf);
                    fflush(stdout);
                } else if (n == 0) {
                    printf("Client disconnected\n");
                    close(sd);
                    clients[i] = -1;
                } else {
                    perror("Read error");
                    close(sd);
                    clients[i] = -1;
                }
            }
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
