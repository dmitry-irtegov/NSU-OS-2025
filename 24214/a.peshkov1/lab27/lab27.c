#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <path-to-file>\n", argv[0]);
        return 2;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    FILE *wc = popen("wc -l", "w");
    if (!wc) {
        perror("popen wc -l");
        close(fd);
        return 1;
    }

    char buf[4096];
    ssize_t bytes_read;
    int seen_nonempty = 0;

    while ((bytes_read = read(fd, buf, sizeof buf)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buf[i] == '\n') {
                if (!seen_nonempty) fputc('\n', wc);
                seen_nonempty = 0;
            } else {
                seen_nonempty = 1;
            }
        }
    }


    if (bytes_read == -1) {
        perror("read");
        close(fd);
        pclose(wc);
        return 1;
    }

    close(fd);

    int status = pclose(wc);
    if (status == -1) {
        perror("pclose");
        return 1;
    }
    return 0;
}