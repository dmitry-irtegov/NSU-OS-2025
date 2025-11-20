#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Incorrect usage, should be:\n%s <command> <args, if needed>\n", argv[0]);
        exit(3);
    }

    pid_t pid = fork();

    if (pid < 0) { //ошибка
        perror("fork");
        exit(1);
    } else if (pid == 0) { //потомок
        printf("Child process started.\n");

        char **subargs = &argv[1];
        execvp(subargs[0], subargs);

        perror("execvp"); // <- в нормальном случае мы не должны доходить до этой части кода
        exit(2);
    } else { //родитель
        printf("Parent process started.\n");

        int status;
        waitpid(pid, &status, 0); //Ожидание завершения потомка

        printf("Child returned value: %d\n", status);
        if (WIFEXITED(status)) {
            printf("Child process exited with code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process terminated by signal: %d\n", WTERMSIG(status));
        }

        printf("Parent process closing...\n");
    }
    return 0;
}
