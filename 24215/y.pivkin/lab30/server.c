#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <ctype.h>

#define SPATH "/tmp/lab30test"
#define BSIZE 256

void to_uppercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

int main() {
    struct sockaddr_un addr_server, addr_client;

    int fd_server = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd_server == -1) {
        perror("socket");
        exit(1);
    }

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    strncpy(addr_server.sun_path, SPATH, sizeof(addr_server.sun_path) - 1);

    unlink(SPATH);

    if(bind(fd_server, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        perror("bind");
        close(fd_server);
        exit(2);
    }

    if(listen(fd_server, 1) == -1) {
        perror("listen");
        close(fd_server);
        exit(3);
    }

    printf("The server is booted, waiting for connection.\n");

    socklen_t len_client = sizeof(addr_client);
    int fd_client = accept(fd_server, (struct sockaddr*)&addr_client, &len_client);
    if(fd_client == -1) {
        perror("accept");
        close(fd_server);
        exit(4);
    }

    printf("Client connected.\n");

    char buffer[BSIZE];
    ssize_t bytes_read = read(fd_client, buffer, BSIZE - 1);
    while (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        to_uppercase(buffer);
        printf("%s", buffer);
        fflush(stdout);

        bytes_read = read(fd_client, buffer, BSIZE - 1);
    }

    if (bytes_read == -1) {
        perror("read");
    }

    printf("Client disconnected.\n");

    close(fd_client);
    close(fd_server);

    unlink(SPATH);
    exit(0);
}
