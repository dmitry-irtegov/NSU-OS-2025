#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SPATH "/tmp/lab30test"
#define BSIZE 256

int main() {
    struct sockaddr_un addr_server;

    int fd_client = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_client == -1) {
        perror("socket");
        exit(1);
    }

    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sun_family = AF_UNIX;
    strncpy(addr_server.sun_path, SPATH, sizeof(addr_server.sun_path) - 1);

    printf("Connecting...\n");

    if (connect(fd_client, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        perror("connect");
        close(fd_client);
        exit(2);
    }

    printf("Connected. Use Ctrl+D to exit:\n");

    char buffer[BSIZE];
    while(fgets(buffer, BSIZE, stdin) != NULL) {
        ssize_t bytes_written = write(fd_client, buffer, BSIZE - 1);

        if (bytes_written == -1) {
            perror("write");
            break;
        }
    }

    printf("Done.\n");

    close(fd_client);
    exit(0);
}
