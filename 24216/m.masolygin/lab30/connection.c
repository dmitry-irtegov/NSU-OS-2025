#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void connection(char* socket_path, int* fd, int type) {
    struct sockaddr_un addr;

    if ((*fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    switch (type) {
        case 0:
            unlink(socket_path);

            if (bind(*fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
                perror("bind error");
                exit(1);
            }
            if (listen(*fd, 5) == -1) {
                perror("listen error");
                exit(1);
            }
            break;
        case 1:
            if (connect(*fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
                perror("connect error");
                exit(1);
            }
            break;
        default:
            break;
    }
}