#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

char* sock_path = "./sock";

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_un serv_addr;
    char buffer[BUFSIZ];

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket creation error");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, sock_path, sizeof(serv_addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        close(sock);
        return 1;
    }

    printf("Connected to the server. Enter the text (Ctrl+D to finish):\n");

    while (fgets(buffer, BUFSIZ, stdin) != NULL) {
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("write error");
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    close(sock);
    return 0;
}
