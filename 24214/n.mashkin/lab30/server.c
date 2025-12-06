#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/uppercase_socket_n.mashkin"
#define BUFFER_SIZE 4096

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(-1);
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    unlink(SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(-1);
    }
    
    if (listen(server_fd, 0) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(-1);
    }
    
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(-1);
    }
    
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = buffer[i] - 'a' + 'A';
            }
        }
        
        if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
            perror("write");
            close(client_fd);
            unlink(SOCKET_PATH);
            exit(-1);
        }
    }
    
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
