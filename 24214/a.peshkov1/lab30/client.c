#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOCK_PATH "/tmp/uds_upper.sock"
#define BUFSZ 4096

static ssize_t write_all(int fd, const void *buf, size_t count) {
    const char *p = buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= (size_t)n;
        p += n;
    }
    return (ssize_t)count;
}

int main(void) {
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(SOCK_PATH) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "SOCK_PATH too long\n");
        return 1;
    }
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        return 1;
    }

    char buf[BUFSZ];
    for (;;) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            return 1;
        }
        if (write_all(sfd, buf, (size_t)n) < 0) {
            perror("write");
            return 1;
        }
    }

    close(sfd);
    return 0;
}
