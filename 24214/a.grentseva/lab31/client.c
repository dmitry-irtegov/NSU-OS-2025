#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Error socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error connect");
        return 1;
    }

    printf("Connected to server. Type text and press Enter (Ctrl+D to quit):\n");

    while (fgets(buffer, sizeof(buffer), stdin)) {
        write(sock_fd, buffer, strlen(buffer));
    }

    close(sock_fd);
    return 0;
}
