#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Wrong arguments.\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
    {
        perror("Couldn't open it");
        exit(1);
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        if (errno == EACCES || errno == EAGAIN) {
            fprintf(stderr, "File is already locked.\n");
        } else {
            perror("fcntl failed");
        }
        exit(1);
    }

    int buff_size = snprintf(NULL, 0, "vi %s", argv[1]) + 1;
    char cmd[buff_size];
    snprintf(cmd, sizeof(cmd), "vi %s", argv[1]);

    if (system(cmd) == -1)
    {
        perror("Couldn't open vim");
        exit(1);
    }

    return 0;
}
