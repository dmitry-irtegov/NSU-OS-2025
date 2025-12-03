#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUF_SIZE 2

void closepipe(int fd[2], int type) {
    if (close(fd[type]) == -1) {
        perror("pipeclose");
        exit(1);
    }
}

int main() {
    int status;
    int fd[2];
    int readc;
    char buf[BUF_SIZE];

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("Error fork");
            exit(1);

        case 0:
            closepipe(fd, 0);

            while ((readc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
                int bytes_written = 0;
                while (bytes_written < readc) {
                    int written = write(fd[1], buf + bytes_written,
                                        readc - bytes_written);
                    if (written < 0) {
                        perror("write error");
                        closepipe(fd, 1);
                        exit(1);
                    }
                    bytes_written += written;
                }
            }

            if (readc == -1) {
                perror("read from stdin");
                closepipe(fd, 1);
                exit(1);
            }
            closepipe(fd, 1);
            exit(0);
        default:
            closepipe(fd, 1);

            while ((readc = read(fd[0], buf, sizeof(buf))) > 0) {
                for (int i = 0; i < readc; i++) {
                    buf[i] = toupper(buf[i]);
                }
                int bytes_written = 0;
                while (bytes_written < readc) {
                    int written = write(STDOUT_FILENO, buf + bytes_written,
                                        readc - bytes_written);
                    if (written < 0) {
                        perror("write error");
                        closepipe(fd, 0);
                        exit(1);
                    }
                    bytes_written += written;
                }
            }
            if (readc == -1) {
                perror("read");
                closepipe(fd, 0);
                exit(1);
            }

            closepipe(fd, 0);

            if (waitpid(pid, &status, 0) == -1) {
                perror("Error waitpid");
                exit(1);
            }

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            } else {
                return 1;
            }
    }

    return 0;
}