#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define SOCKET_PATH "mysocket" 
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t num_read;

    //Создаем сокет
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }

    //Настраиваем адрес
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    //Удаляем старый файл сокета, если он есть
    unlink(SOCKET_PATH);

    //Привязываем сокет (bind)
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(1);
    }

    //Слушаем
    if (listen(server_fd, 5) == -1) {
        perror("listen error");
        exit(1);
    }

    printf("Server listening on '%s'...\n", SOCKET_PATH);

    //Принимаем соединение
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
        perror("accept error");
        exit(1);
    }

    //Читаем данные, переводим в UpperCase и печатаем
    while ((num_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        for (int i = 0; i < num_read; i++) {
            buffer[i] = toupper((unsigned char)buffer[i]);
        }
        write(STDOUT_FILENO, buffer, num_read);
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}
