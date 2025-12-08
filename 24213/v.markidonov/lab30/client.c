#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define BUFFER_SIZE 1024

int main() {
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/somesocket");

    if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t readed = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        if (readed > 0) {
            ssize_t sended = send(socket_fd, buffer, readed, 0);
            if (sended < 0) {
                perror("send");
                exit(1);
            }
        } else if (readed == 0) {
            break;
        } else {
            perror("read");
            exit(1);
        }
    }
    
    return 0;
}
