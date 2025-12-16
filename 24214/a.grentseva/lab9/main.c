#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    char *cmd_argv[] = {"cat", "/etc/protocols", NULL};
    pid_t child_process;

    printf("[Parent]: Начинаю выполнение программы...\n");

    switch (child_process = fork()) {
    case -1:
        perror("Failed to create child process");
        return 1;

    case 0:
        execvp(cmd_argv[0], cmd_argv);        
        perror("Failed to run cat");
        return 1;

    default: {
        printf("[Parent]: Текст, напечатанный ДО завершения ребенка.\n");

        int child_status;
        if (wait(&child_status) == -1) {
            perror("wait failed");
            return 1; 
        }

        printf("\n--- Конец вывода подпроцесса ---\n");
        printf("[Parent] Текст, напечатанный ПОСЛЕ завершения ребенка.\n");
    }
    }
    return 0;
}
