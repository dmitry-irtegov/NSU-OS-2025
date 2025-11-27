#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/my_socket"
#define BUF 1024

int main() {
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd < 0) { 
        perror("socket"); 
        exit(EXIT_FAILURE); 
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

    unlink(SOCKET_PATH);
    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        close(sfd);
        exit(EXIT_FAILURE);
    }
    if (listen(sfd, 1) < 0) {
        perror("listen"); 
        close(sfd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер ожидает...\n");

    int cfd = accept(sfd, NULL, NULL);
    if (cfd < 0) { 
        perror("accept"); 
        close(sfd);
        exit(EXIT_FAILURE); 
    }

    char buf[BUF];
    ssize_t n;
    while ((n = read(cfd, buf, BUF)) > 0) {
        for (int i = 0; i < n; i++)
            buf[i] = toupper((unsigned char)buf[i]);

        if (write(STDOUT_FILENO, buf, n)) {
            perror("write to stdout");
            break;
        }
    }

    if (n == -1) {
        perror("read from client");
    }

    printf("\nКлиент отключился. Завершение.\n");

    close(cfd);
    close(sfd);
    unlink(SOCKET_PATH);
    return 0;
}
