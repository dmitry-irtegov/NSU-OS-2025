#include <stdio.h>
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
        return 1;
    case 0:
        execlp("cat", "cat", NAMEFILE, NULL);
        perror("Не удалось выполнить cat");
        return 1;
    default:
        waitpid(pid, NULL, 0);
        printf("\nРодительский процесс\n");
        break;
    }
    return 0;
}