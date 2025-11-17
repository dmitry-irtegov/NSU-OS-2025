#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <poll.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SOCKFILE "socket"
#define MAXC 10
#define BUFS 4096

int work = 1;

void stop(int s) {
    (void)s;
    work = 0;
}

int main(void) {
    int srv;
    struct sockaddr_un sa;
    struct pollfd p[MAXC + 1];
    char buf[BUFS];
    int i;

    signal(SIGINT, stop);
    signal(SIGTERM, stop);

    unlink(SOCKFILE);

    srv = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, SOCKFILE, sizeof(sa.sun_path) - 1);

    if (bind(srv, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(srv);
        return 1;
    }

    if (listen(srv, MAXC) < 0) {
        perror("listen");
        close(srv);
        unlink(SOCKFILE);
        return 1;
    }

    p[0].fd = srv;
    p[0].events = POLLIN;

    for (i = 1; i <= MAXC; i++) {
        p[i].fd = -1;
        p[i].events = POLLIN;
    }

    while (work) {
        int r = poll(p, MAXC + 1, -1);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }

        if (p[0].revents & POLLIN) {
            int c = accept(srv, NULL, NULL);
            if (c >= 0) {
                int sl = 0;
                for (i = 1; i <= MAXC; i++) {
                    if (p[i].fd == -1) {
                    p[i].fd = c;
                    sl = 1;
                    break;
                    }
                }
                if (!sl) {
                    close(c);
                }
            }
        }

        for (i = 1; i <= MAXC; i++) {
            if (p[i].fd == -1) {
                continue;
            }

            if (p[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                close(p[i].fd);
                p[i].fd = -1;
                continue;
            }

            if (p[i].revents & POLLIN) {
                ssize_t n = read(p[i].fd, buf, BUFS);
                if (n <= 0) {
                    close(p[i].fd);
                    p[i].fd = -1;
                } else {
                    for (ssize_t j = 0; j < n; j++){
                        buf[j] = toupper((unsigned char)buf[j]);
                    }
                    write(STDOUT_FILENO, buf, n);
                    fsync(STDOUT_FILENO);
                }
            }
        }
    }

    for (i = 1; i <= MAXC; i++){
        if (p[i].fd != -1) {
            close(p[i].fd);
        }
    }

    close(srv);
    unlink(SOCKFILE);
    return 0;
}
