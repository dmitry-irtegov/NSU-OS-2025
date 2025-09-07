#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/uds_lab30.sock"

int main(void) {
    int srv = -1, cli = -1;
    struct sockaddr_un addr;
    char buf[4096];

    srv = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCKET_PATH);

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(srv);
        return 1;
    }
    if (listen(srv, 1) < 0) {
        perror("listen");
        close(srv);
        unlink(SOCKET_PATH);
        return 1;
    }

    cli = accept(srv, NULL, NULL);
    if (cli < 0) {
        perror("accept");
        close(srv);
        unlink(SOCKET_PATH);
        return 1;
    }

    for(;;) {
        ssize_t n = recv(cli, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            break;
        }
        if (n == 0) {
            break;
        }
        for (ssize_t i = 0; i < n; ++i) {
            buf[i] = (char)((unsigned char)buf[i]);
        }
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(STDOUT_FILENO, buf + off, (size_t)(n - off));
            if (w < 0) {
                if (errno == EINTR) continue;
                perror("write");
                break;
            }
            off += w;
        }
    }

    close(cli);
    close(srv);
    unlink(SOCKET_PATH);
    return 0;
}
