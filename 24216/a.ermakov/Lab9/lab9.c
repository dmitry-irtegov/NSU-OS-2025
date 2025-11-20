#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
    pid_t child_pid = fork();
    int status;
    pid_t wait_result;

    switch (child_pid) {
        case 0:
            execlp("cat", "cat", "test.txt", (char *)NULL);
            perror("Ошибка запуска программы cat");
            exit(EXIT_FAILURE);

        case -1:
            perror("Не удалось создать дочерний процесс");
            exit(EXIT_FAILURE);

        default:
            wait_result = waitpid(child_pid, &status, 0);
            switch (wait_result) {
                case 0:
                    break;

                case -1:
                    perror("Сбой при ожидании завершения процесса");
                    exit(EXIT_FAILURE);

                default:
                    printf("\nЭтап предварительной обработки успешно завершён.\n");
            }
    }

    return 0;
}
