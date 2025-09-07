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

        unsigned char ch;
        ssize_t n;
        while ((n = read(fd[0], &ch, 1)) > 0) {
            ch = (unsigned char)toupper(ch);
            write(STDOUT_FILENO, &ch, 1);
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
