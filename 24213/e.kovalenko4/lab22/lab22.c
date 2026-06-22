#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>

#define TIME_OUT 5
#define BUF_SIZE 256

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        return 1;
    }

    int active = 1;
    int *fd = malloc((argc - 1) * sizeof(int));
    int *eof = calloc(argc - 1, sizeof(int));
    char buf[BUF_SIZE];

    for (int i = 0; i < argc - 1; i++) {
        fd[i] = open(argv[i + 1], O_RDONLY);
        if (fd[i] < 0) {
            perror(argv[i + 1]);
            eof[i] = 1;
        }
    }

    while (active) {

        active = 0;
        for (int i = 0; i < argc - 1; i++) {
            if (eof[i]) {
                continue;
            }

            fd_set rfds;
            struct timeval tv;

            FD_ZERO(&rfds);
            FD_SET(fd[i], &rfds);

            tv.tv_sec = TIME_OUT;
            tv.tv_usec = 0;

            int ret = select(fd[i] + 1, &rfds, NULL, NULL, &tv);

            if (ret > 0) {
                ssize_t n = read(fd[i], buf, BUF_SIZE - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    printf("[%s] %s\n", argv[i + 1], buf);
                } else {
                    eof[i] = 1;
                    close(fd[i]);
                }
            }

            active = 1;
        }

    }

    free(fd);
    free(eof);
    return 0;
}
