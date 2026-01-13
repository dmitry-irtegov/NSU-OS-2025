#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/mysocket"
#define BUF_SIZE 1024

int main(void) {
    int sockfd;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    ssize_t n;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    //адрес
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(1);
    }

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        if (send(sockfd, buf, n, 0) == -1) {
            perror("send");
            exit(1);
        }
    }
    if (n == -1) {
        perror("read");
        exit(1);
    }

    close(sockfd);
    return 0;
