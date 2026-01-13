#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>

int main(void) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(fd[1]);

        char buf[100];
        ssize_t n;

        while ((n = read(fd[0], buf, 100)) > 0) {
            for(ssize_t i = 0; i < n; i++) {
                buf[i] = (char)toupper((unsigned char) buf[i]);
            }
            write(STDOUT_FILENO, buf, n);
        }

        close(fd[0]);
        _exit(EXIT_SUCCESS);

    } else {
        close(fd[0]);

        const char *text = "Some usual text\n";
        write(fd[1], text, strlen(text));
        close(fd[1]);
        waitpid(pid, NULL, 0);
    }

    return 0;
}
