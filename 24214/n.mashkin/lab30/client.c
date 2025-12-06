#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/uppercase_socket_n.mashkin"
#define BUFFER_SIZE 4096

int main() {
    int client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(-1);
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(-1);
    }
    
    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(client_fd, buffer, bytes_read);
        if (bytes_written == -1) {
            perror("write");
            close(client_fd);
            exit(-1);
        }
    }
    
    close(client_fd);
    return 0;
}
