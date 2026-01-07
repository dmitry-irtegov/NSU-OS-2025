#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#define BUFFER_SIZE 4096

int main() {
    int fd[2];
    char data[BUFFER_SIZE];
    pid_t pid;

    if (pipe(fd) == -1) {
        perror("pipe error");
        return -1;
    }
    ssize_t count;

    if ((pid = fork()) > 0) {
        close(fd[1]);
        while ((count = read(fd[0], data, BUFFER_SIZE)) > 0) {
            for (ssize_t i = 0; i < count; i++) {
                data[i] = toupper((unsigned char)data[i]);
            }
            if (write(STDOUT_FILENO, data, count) == -1) {
                perror("write error");
                if (wait(NULL) == -1) {
                    perror("wait error");
                }
                return -1;
            }
        }
        if (count == -1) {
            perror("read error");
            if (wait(NULL) == -1) {
                perror("wait error");
            }
            return -1;
        }

        if (wait(NULL) == -1) {
            perror("wait error");
            return -1;
        }
        return 0;
    } else if (pid == 0) {
        close(fd[0]);
        while ((count = read(STDIN_FILENO, data, BUFFER_SIZE)) > 0) {
            if (write(fd[1], data, count) != count) {
                perror("write error");
                return -1;
            }
        }
        if (count == -1) {
            perror("read error");
            return -1;
        }
        close(fd[1]);
        return 0;
    } else {
        perror("fork error");
        return -1;
    }
}
