#include "origin.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int origin_connect_start(url_t* url) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res;
    struct addrinfo* rp;
    if (getaddrinfo(url->host, url->port, &hints, &res) != 0) {
        return -1;
    }

    int server = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        server = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (server < 0) {
            break;
        }

        int flags = fcntl(server, F_GETFL, 0);
        fcntl(server, F_SETFL, flags | O_NONBLOCK);

        int rc = connect(server, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0 || errno == EINPROGRESS) {
            break;
        }

        close(server);
        server = -1;
    }
    freeaddrinfo(res);

    return server;
}

void origin_set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}
