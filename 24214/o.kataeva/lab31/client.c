#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFS 4096

int main(void) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, "socket");

    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    char buf[BUFS];
    while (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        size_t pos = 0;
        while (pos < len) {
            ssize_t w = write(fd, buf + pos, len - pos);
            if (w < 0) {
                perror("write");
                close(fd);
                return 1;
            }
            pos += w;
        }
    }

    close(fd);
    return 0;
}
