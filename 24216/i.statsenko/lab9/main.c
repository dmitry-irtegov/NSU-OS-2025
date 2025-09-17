#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define NAMEFILE "file.txt"

int main(void)
{
    pid_t pid = fork();
    switch (pid)
    {
    case -1:
        perror("Ошибка при создании процесса");
        exit(EXIT_FAILURE);
    case 0:
        execlp("cat", "cat", NAMEFILE, NULL);
        perror("Не удалось выполнить cat");
        exit(EXIT_FAILURE);
    default:
        if (waitpid(pid, NULL, 0) == -1) {
            perror("Ошибка при ожидании завершения дочернего процесса");
            exit(EXIT_FAILURE);
        }
        printf("\nРодительский процесс\n");
        break;
    }
    return 0;
}