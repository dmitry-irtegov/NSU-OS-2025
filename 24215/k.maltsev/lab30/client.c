#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "mysocket"
#define BUFFER_SIZE 1024

int main() {
    int sfd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t num_read;

    //Создаем сокет
    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }

    //Настраиваем адрес
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    //Соединяемся (connect)
    if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        exit(1);
    }

    printf("Connected. Type text:\n");

    //Читаем с клавиатуры и шлем в сокет
    while ((num_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        if (write(sfd, buffer, num_read) != num_read) {
            perror("write error");
            exit(1);
        }
    }

    close(sfd);
    return 0;
}
