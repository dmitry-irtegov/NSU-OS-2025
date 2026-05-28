#define _GNU_SOURCE
#include "utils.h"

int fd_to_index[MAX_FDS * 2];  

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void safe_close(int fd) {
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

int find_free_slot(void) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (pollfds[i].fd < 0) return i;
    }
    return -1;
}

void parse_path(const char *url, char *path, size_t path_sz) {
    const char *p = strstr(url, "://");
    p = p ? p + 3 : url;
    const char *slash = strchr(p, '/');
    if (slash) {
        strncpy(path, slash, path_sz - 1);
        path[path_sz - 1] = '\0';
    } else {
        strncpy(path, "/", path_sz - 1);
        path[path_sz - 1] = '\0';
    }
}

int resolve_host(const char *host, int port, struct sockaddr_in *addr) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return -1;
    }
    memcpy(addr, res->ai_addr, sizeof(*addr));
    freeaddrinfo(res);
    return 0;
}