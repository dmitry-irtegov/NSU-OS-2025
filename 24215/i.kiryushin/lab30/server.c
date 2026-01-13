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

void SIGINT_handler(int signum) {
    unlink(socket_path);
    exit(EXIT_SUCCESS);
}


int main(){
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (sockfd == -1) {
        perror("error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
        perror("error setting up signal handler");
        close(sockfd);
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }
    
    if (listen(sockfd, 1) == -1) {
        perror("error listening on socket");
        close(sockfd);
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    int connectedfd = accept(sockfd, NULL, NULL);

    if (connectedfd == -1) {
        perror("error accepting connection");
        close(sockfd);
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    char buffer[SIZE];

    ssize_t numRead = 0;
    while ((numRead = read(connectedfd, buffer, SIZE)) > 0) {
        for (int i = 0; i < numRead; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        write(STDOUT_FILENO, buffer, numRead);
    }

    if (numRead == -1) {
        perror("error reading from socket");
        close(connectedfd);
        close(sockfd);
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    close(connectedfd);
    close(sockfd);
    unlink(socket_path);
    return 0;
}
