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
#define BUF_SIZE 1024

int main() {
    int srvfd, clifd;
    struct sockaddr_un addr;
    char buf[BUF_SIZE];
    ssize_t n;

    unlink(SOCKET_PATH);

    srvfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srvfd == -1) {
        perror("socket");
        exit(1);
    }

    //адрес
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    //завершаем строку нулём
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(srvfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(srvfd, 1) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on %s\n", SOCKET_PATH);

    clifd = accept(srvfd, NULL, NULL);
    if (clifd == -1) {
        perror("accept");
        exit(1);
    }

    while ((n = recv(clifd, buf, sizeof(buf), 0)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            buf[i] = (char)toupper((unsigned char)buf[i]);
        }
        if (write(STDOUT_FILENO, buf, n) == -1) {
            perror("write");
            exit(1);
        }
    }
    if (n == -1) {
        perror("recv");
        exit(1);
    }

    close(clifd);
    close(srvfd);
    unlink(SOCKET_PATH);
    return 0;
}
