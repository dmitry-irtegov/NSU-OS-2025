#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/unix_socket_lab30"
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Failed to create client socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Unable to connect to /tmp/unix_socket_lab30");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключено к серверу\n");
    printf("Введите текст (Ctrl+D для завершения):\n");
    
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        if (write(client_fd, buffer, bytes_read) == -1) {
            perror("Failed to send data to server");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    }
    
    if (bytes_read == -1) {
        perror("Unable to read input from stdin");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Отправка завершена. Отключение от сервера...\n");
    close(client_fd);
    
    return 0;
}
