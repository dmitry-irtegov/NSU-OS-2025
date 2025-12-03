#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid;

    // Создаем подпроцесс
    pid = fork();

    if (pid < 0) {
        // Ошибка при создании подпроцесса
        fprintf(stderr, "Fork failed\n");
        return 1;

    } else if (pid == 0) {
        // Это подпроцесс
        printf("Child process: Executing cat on long_file.txt\n");
        execlp("cat", "cat", "long_file.txt", NULL);

        fprintf(stderr, "Child process: Exec failed\n");
        exit(1);
        
    } else {
        // Это родительский процесс
        printf("Parent process: Child process created with PID %d\n", pid);
        printf("Parent process: This is some text from the parent before child completion.\n");

        int status;
        waitpid(pid, &status, 0);

        printf("Parent process: Child process with PID %d has terminated.\n", pid);
        printf("Parent process: This is the last line printed by the parent after child completion.\n");
    }

    return 0;
}

