#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "использование: %s <команда> [аргументы...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();

    switch (child) {
        case 0: {
            execvp(argv[1], &argv[1]); 

            perror("ошибка execvp");
            exit(EXIT_FAILURE); 
        }

        case -1:
            perror("ошибка fork");
            exit(EXIT_FAILURE);

        default: {
            int status_store;

            pid_t result = waitpid(child, &status_store, 0);
            
            if (result == -1) {
                perror("ошибка waitpid");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(status_store)) {
                int exit_code = WEXITSTATUS(status_store);
                printf("\nкоманда завершилась с кодом: %d\n", exit_code);
            } else if (WIFSIGNALED(status_store)) {
                printf("\nкоманда была завершена сигналом: %d\n", WTERMSIG(status_store));
            }
            
            return EXIT_SUCCESS;
        }
    }
}