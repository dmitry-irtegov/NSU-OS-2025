#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/mysocket"
#define MAXLINE 1024

int main(void) {
    int sockfd;
    struct sockaddr_un servaddr;
    char buf[MAXLINE];
    ssize_t n;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, SOCKET_PATH, sizeof(servaddr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }

    while ((n = read(STDIN_FILENO, buf, MAXLINE)) > 0) {
        if (send(sockfd, buf, n, 0) != n) {
            perror("send");
            exit(1);
        }
    }
    
    if (n < 0) {
        perror("read");
        exit(1);
    }

    close(sockfd);
    return 0;
}
