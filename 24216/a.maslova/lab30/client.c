#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024

int create_client_socket() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return -1;
    }
    
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

int send_data_to_server(int sockfd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        if (write(sockfd, buffer, bytes_read) == -1) {
            return -1;
        }
    }

    if (bytes_read == -1) {
        return -1;
    }

    return 0;
}

void cleanup_client(int sockfd) {
    if (sockfd != -1) {
        close(sockfd);
    }
}

int main() {
    int sockfd = -1;
    
    sockfd = create_client_socket();
    if (sockfd == -1) {
        perror("Error connecting to server");
        cleanup_client(sockfd);
        return 1;
    }
    
    printf("Подключено к серверу. Введите текст (Ctrl+D для завершения):\n");
    
    if (send_data_to_server(sockfd) == -1) {
        perror("Error sending data to server");
        cleanup_client(sockfd);
        return 1;
    };
    
    printf("Соединение закрыто\n");
    
    cleanup_client(sockfd);
    
    return 0;
}