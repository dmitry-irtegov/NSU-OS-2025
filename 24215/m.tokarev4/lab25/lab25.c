#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    siginfo_t  info;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[1]);
        printf("Дочерний процесс: полученный текст в верхнем регистре:\n");

        while ((bytes_read = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                buffer[i] = toupper(buffer[i]);
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);

    }
    else {
        close(pipefd[0]);
        const char* text ="Тестовая строка\n i want to pass the task for one class\nPu pu puuu\n I will think about it\nКОНЕЦ СООБЩЕНИЯ.\n";

        write(pipefd[1], text, strlen(text));
	close(pipefd[1]);
        if (waitid(P_PID, pid, &info, WEXITED) == -1) {
	    perror("waitid");
	    exit(EXIT_FAILURE);
	}
//	close(pipefd[1]);
        printf("Родительский процесс: работа завершена.\n");
    }

    return 0;
}
