#include "listener.h"

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int listener_open(char* port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* res;
    struct addrinfo* rp;
    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        fprintf(stderr, "Error: cannot resolve port %s\n", port);
        return -1;
    }

    int listen_fd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd < 0) {
            break;
        }

        int yes = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        close(listen_fd);
        listen_fd = -1;
    }
    freeaddrinfo(res);

    if (listen_fd < 0) {
        fprintf(stderr, "Error: cannot bind to port %s\n", port);
        return -1;
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int listener_accept(int listen_fd) {
    int client = accept(listen_fd, NULL, NULL);
    if (client < 0) {
        perror("accept");
    }
    return client;
}
