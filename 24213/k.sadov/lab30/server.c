#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#define LISTEN_BACKLOG 1
#define BUF_SIZE 1024
const char *socket_path = "/tmp/u_socket";

void cleanup_handler(int sig) {
    unlink(socket_path);
    exit(EXIT_SUCCESS);
}

int main() {

    int sfd, cfd;
    struct sockaddr_un my_addr;

    signal(SIGINT, cleanup_handler);

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, '\0', sizeof(struct sockaddr_un));
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path, socket_path, sizeof(my_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        perror("listen");
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    cfd = accept(sfd, NULL, NULL);
    if (cfd == -1) {
        perror("accept");
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    char buff[BUF_SIZE];
    ssize_t read_bytes;
    while ((read_bytes = read(cfd, buff, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < read_bytes; i++) {
            buff[i] = toupper((unsigned char)buff[i]);
        }

        if (write(STDOUT_FILENO, buff, read_bytes) == -1) {
            perror("write");
            unlink(socket_path);
            exit(EXIT_FAILURE);
        }
    }

    if (read_bytes == -1) {
        perror("read fail");
        unlink(socket_path);
        exit(EXIT_FAILURE);
    }

    close(sfd);
    close(cfd);
    unlink(socket_path);
    exit(EXIT_SUCCESS);
}
