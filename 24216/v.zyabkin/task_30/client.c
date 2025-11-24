#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET "/tmp/test_socket_zyabkin"

int main() {
    const char *test_msg = "This is a creative message. I beg you, merge this LAB!!!\n";

    int sock;
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("failed to create socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sun_path, SOCKET);
    addr.sun_family = AF_UNIX;

    int connection_res = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (connection_res == -1) {
        perror("failed to connect");
        close(sock);
        exit(1);
    }

    ssize_t m = write(sock, test_msg, strlen(test_msg));
    if (m == -1) {
        perror("failed to write");
        close(sock);
        exit(-1);
    }

    close(sock);
    return 0;
}