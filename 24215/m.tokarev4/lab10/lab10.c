#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <команда> [аргументы...]\n", argv[0]);
        exit(1);
    }

    pid_t pid = fork();
    siginfo_t info;

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        printf("Дочерний процесс: это говорю я\n");

        execvp(argv[1], &argv[1]);

        perror("execvp failed");
        exit(1);
    } else {
        printf("Родительский процесс: ожидаем завершения дочернего процесса...\n");

        if (waitid(P_PID, pid, &info, WEXITED) == -1) {
            perror("waitid failed");
        } else {
            printf("Родительский процесс: дочерний процесс %d завершился, код возвращения %d\n", info.si_pid, info.si_status);
        }
    }

    return 0;
}
