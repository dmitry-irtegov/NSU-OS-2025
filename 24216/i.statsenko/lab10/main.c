#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


// говно не робит
int main(int argc, char *argv[]) {
    int code_exit;
    pid_t pid = fork();
    switch (pid)
    {
    case -1:
        perror("Fork failed");
        break;
    case 0:
        if (argc > 1) {
            code_exit = execv(argv[1], &argv[1]);
        } 
        else {
            printf("Отсутсвует название программы для запуска\n");
        }
    default:
        waitpid(pid, NULL, 0);
        printf("Код завершения: %d\n", code_exit);
        break;
    }
}