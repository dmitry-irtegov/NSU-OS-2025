#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    siginfo_t info;
    
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }
    
    if (pid == 0) {
        printf("Дочерний процесс: создание и вывод файла\n");
        FILE* file = fopen("long_file.txt", "w");
        if (file == NULL) {
            perror("fopen failed");
            exit(1);
        }
        
        for (int i = 1; i <= 100; i++) {
            fprintf(file, "%d\n", i);
        }
        fclose(file);
        
        execlp("cat", "cat", "long_file.txt", NULL);
        
        perror("execlp failed");
        exit(1);
    } else {
        printf("Родительский процесс: ожидаем завершения дочернего процесса...\n");
        
        if (waitid(P_PID, pid, &info, WEXITED) == -1) {
            perror("waitid failed");
        } else {
            printf("Родительский процесс: дочерний процесс %d завершился\n", info.si_pid);
        }
    }
    
    return 0;
}

