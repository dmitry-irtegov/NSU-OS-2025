#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>

#define SIZE 1024

char* socket_path = "mysocket";

int main() {

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (sockfd == -1) {
        perror("error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("error connecting to socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[SIZE];
    ssize_t num_bytes = 0;
    while ((num_bytes = read(STDIN_FILENO, buffer, SIZE)) > 0) { 
        char *ptr = buffer;
        ssize_t remaining = num_bytes;

        while (remaining > 0) {
            ssize_t written = write(sockfd, ptr, remaining);
            if (written == -1) {
                perror("error writing to socket");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            ptr += written;
            remaining -= written;
        }
    }

    if (num_bytes == -1) {
        perror("error reading from stdin");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
