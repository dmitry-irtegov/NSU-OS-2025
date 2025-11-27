#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h> 

#define SOCKET_PATH "/tmp/lab31_socket"
#define BUFF_SIZE 256

int main() {
    int sock_fd;
    struct sockaddr_un addr;
    char buffer[BUFF_SIZE];
    ssize_t bytes_read;

    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socet failed");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected. My PID: %d. Sending text...\n", getpid());

    while ((bytes_read = read(STDOUT_FILENO, buffer, BUFF_SIZE)) > 0) {
        write(sock_fd, buffer, bytes_read);
    }

    close(sock_fd);
    return 0;
}