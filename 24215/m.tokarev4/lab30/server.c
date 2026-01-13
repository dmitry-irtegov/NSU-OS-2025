#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#define SOCKET_PATH "./socket"
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    int bytes_cnt;
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %s\n", SOCKET_PATH);

    client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected\n");
    while ((bytes_cnt = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_cnt] = '\0';
        for (int i = 0; i < bytes_cnt; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = toupper(buffer[i]);
            }
        }
	printf("%s", buffer);
	fflush(stdout);
    }

    printf("\nClient disconnected, connection closed\n");
    close(client_fd);
    close(server_fd);

    unlink(SOCKET_PATH);

    return 0;
}
