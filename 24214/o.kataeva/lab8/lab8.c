#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

int run_editor(const char* filepath) {
    const char* editor = getenv("EDITOR");
    if (editor == NULL) {
        editor = "vim";
    }

    int len = snprintf(NULL, 0, "%s %s", editor, filepath) + 1;
    char *command = malloc(len);
    if (!command) {
        perror("malloc failed");
        return -1;
    }

    snprintf(command, len, "%s %s", editor, filepath);

    int status = system(command);
    free(command);

    if (status == -1) {
        perror("failed to execute editor");
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("error opening file");
        return 1;
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        if (errno == EAGAIN || errno == EACCES) {
            fprintf(stderr, "File '%s' is already locked by another process.\n", argv[1]);
        } else {
            perror("fcntl error");
        }
        close(fd);
        return 1;
    }

    if (run_editor(argv[1]) != 0) {
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}