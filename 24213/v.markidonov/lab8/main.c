#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

void run_text_editor(const char* editor, const char* filepath) {
    int command_len = snprintf(NULL, 0, "%s %s", editor, filepath);
    if (command_len == -1) {
        perror("command for run text editor is too long");
        exit(1);
    }
    command_len += 1;
    char command[command_len];
    snprintf(command, command_len, "%s %s", editor, filepath);

    if (system(command) == -1) {
        perror("can't run text editor");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror(argv[1]);
        exit(1);
    }

    struct flock lock = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno == EAGAIN || errno == EACCES) {
            perror("can't lock file, because it's already locked");
        } else {
            perror("fcntl");
        }
        exit(1);
    }

    run_text_editor("vim", argv[1]);
    
    return 0;
}
