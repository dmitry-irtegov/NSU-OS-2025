#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET "/tmp/test_socket_zyabkin"

const int buffer_size = 256;

int main() {
    int server;
    int client;
    struct sockaddr_un addr;
    char buffer[buffer_size];

    unlink(SOCKET);

    server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server < 0) {
        perror("failed to create socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sun_path, SOCKET);
    addr.sun_family = AF_UNIX;

    int bind_res = bind(server, (struct sockaddr*)&addr, sizeof(addr));
    if (bind_res == -1) {
        perror("failed to bind socket");
        close(server);
        exit(1);
    }

    int listen_res = listen(server, 5);
    if (listen_res == -1) {
        perror("failed to listen server");
        close(server);
        exit(1);
    }

    client = accept(server, NULL, NULL);
    if (client == -1) {
        perror("failed to accept connections");
        close(server);
        exit(1);
    }

    ssize_t m;
    while ((m = read(client, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < m; i++) {
            buffer[i] = toupper((unsigned char) buffer[i]);
        }

        buffer[m] = '\0';
        printf("%s\n", buffer);
    }

    close(server);
    close(client);
    unlink(SOCKET);

    return 0;
}
