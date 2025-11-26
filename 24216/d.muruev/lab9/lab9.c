#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    pid_t child = fork();

    switch (child) {
        case 0: {
            char *args[] = {"cat", "file", NULL};

            execvp(args[0], args); 
            
            perror("ошибка execvp");
            exit(EXIT_FAILURE); 
        }

        case -1:
            perror("ошибка fork");
            exit(EXIT_FAILURE);

        default: {
            printf("Запустил потомка с ID = %d\n", child); 
            
            pid_t result = waitpid(child, NULL, 0);
            
            if (result == -1) {
                perror("ошибка waitpid");
                exit(EXIT_FAILURE);
            }

            printf("Потомок %d завершился.\n", result);
            
            exit(EXIT_SUCCESS);
        } 
    } 
}