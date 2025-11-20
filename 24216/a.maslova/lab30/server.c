#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024
#define BACKLOG 5

int create_server_socket() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        return -1;
    }
    
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    unlink(SOCKET_PATH);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, BACKLOG) == -1) {
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

int accept_client_connection(int server_fd) {
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    return client_fd;
}

int process_client_data(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        for (int i = 0; i < bytes_read; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        
        printf("%s", buffer);
        fflush(stdout);
    }

    if (bytes_read == -1) {
        return -1;
    }

    return 0;
}

void cleanup_server(int server_fd, int client_fd) {
    if (client_fd != -1) {
        close(client_fd);
    }
    if (server_fd != -1) {
        close(server_fd);
    }
    unlink(SOCKET_PATH);
}

int main() {
    int server_fd = -1;
    int client_fd = -1;
    
    server_fd = create_server_socket();
    if (server_fd == -1) {
        perror("Error creating server socket");
        return 1;
   }
    
    printf("Сервер запущен и слушает на %s\n", SOCKET_PATH);
    
    client_fd = accept_client_connection(server_fd);
    if (client_fd == -1) {
        perror("Error accepting client connection");
        cleanup_server(server_fd, -1);
        return 1;
    }
    
    printf("Клиент подключен\n");
    
    if (process_client_data(client_fd) == -1) {
        perror("Error processing client data");
        cleanup_server(server_fd, client_fd);
        return 1;
    }
    
    printf("\nСоединение разорвано клиентом\n");
    
    cleanup_server(server_fd, client_fd);
    
    return 0;
}