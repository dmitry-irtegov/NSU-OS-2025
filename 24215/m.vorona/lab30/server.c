#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/mysocket"

int main(void) {
    int sockfd, clifd;
    struct sockaddr_un servaddr, cliaddr;
    socklen_t clilen;
    char buf[1024];
    int n;

    unlink(SOCKET_PATH);

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, SOCKET_PATH, sizeof(servaddr.sun_path) - 1);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("Server listening on %s (PID: %d)\n", SOCKET_PATH, getpid());
    if (listen(sockfd, 1) < 0) {
        perror("listen");
        exit(1);
    }

    clilen = sizeof(cliaddr);
    if ((clifd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
        perror("accept");
        exit(1);
    }

    ssize_t total_written = 0;
    while (total_written < n) {
        ssize_t written = write(STDOUT_FILENO, buf + total_written, n - total_written);
        if (written < 0) {
            perror("write");
            exit(1);
        }
        total_written += written;
    }

    if (n < 0) {
        perror("recv");
        exit(1);
    }

    close(clifd);
    close(sockfd);
    unlink(SOCKET_PATH);
    return 0;
}
