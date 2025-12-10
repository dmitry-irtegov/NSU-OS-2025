#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main() {
    const char* text = "Тестовая строка\n i want to pass the task for one class\nPu pu puuu\n I will think about it\nКОНЕЦ СООБЩЕНИЯ.\n";
    char buffer[1024];

    int pipefd[2];
    
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        close(pipefd[1]);
        close(pipefd[0]);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        close(pipefd[1]);

        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dub2 failed");
            close(pipefd[0]);
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);

        execlp("tr", "tr", "a-z", "A-Z", NULL);

        perror("execlp failed");
        return EXIT_FAILURE;
    }

    close(pipefd[0]);
    int bytes = write(pipefd[1], text, strlen(text));
    if (bytes == -1) {
        perror("write failed");
        close(pipefd[1]);
        return(EXIT_FAILURE);
    }
    close(pipefd[1]);
    wait(NULL);

    return EXIT_SUCCESS;
}

