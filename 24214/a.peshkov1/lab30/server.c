#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOCK_PATH "/tmp/uds_upper.sock"
#define BUFSZ 4096

static void cleanup(void) { unlink(SOCK_PATH); }

static void on_signal(int sig) {
    (void)sig;
    exit(0); // atexit() вызовет cleanup()
}

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
    int sfd = -1, cfd = -1;
    struct sockaddr_un addr;

    atexit(cleanup);
    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    // на всякий случай уберём старый путь, иначе bind выдаст EADDRINUSE.
    unlink(SOCK_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(SOCK_PATH) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "SOCK_PATH too long\n");
        return 1;
    }
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }
    if (listen(sfd, 1) == -1) {
        perror("listen");
        return 1;
    }

    if ((cfd = accept(sfd, NULL, NULL)) == -1) {
        perror("accept");
        return 1;
    }

    char buf[BUFSZ];
    for (;;) {
        ssize_t n = read(cfd, buf, sizeof(buf));
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("read");
            return 1;
        }
        for (ssize_t i = 0; i < n; i++) {
            unsigned char ch = (unsigned char)buf[i];
            buf[i] = (char)toupper(ch);
        }
        if (write_all(STDOUT_FILENO, buf, (size_t)n) < 0) {
            perror("write");
            return 1;
        }
    }

    close(cfd);
    close(sfd);
    return 0;
}
