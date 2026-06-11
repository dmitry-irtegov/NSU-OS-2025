#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SEM_PARENT "/sem_parent_task16"
#define SEM_CHILD  "/sem_child_task16"

int main() {
    // удаляем семафоры если они остались в системе от прошлого запуска
    sem_unlink(SEM_PARENT);
    sem_unlink(SEM_CHILD);

    sem_t *sem_parent = sem_open(SEM_PARENT, O_CREAT | O_EXCL, 0644, 1);
    if (sem_parent == SEM_FAILED) {
        perror("Ошибка sem_open для родителя");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_child = sem_open(SEM_CHILD, O_CREAT | O_EXCL, 0644, 0);
    if (sem_child == SEM_FAILED) {
        perror("Ошибка sem_open для потомка");
        sem_unlink(SEM_PARENT);
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();

    if (pid < 0) {
        perror("Ошибка вызова fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        //Дочерний процесс
        for (int i = 0; i < 10; i++) {
            sem_wait(sem_child);
            printf("Дочерний процесс: %d\n", i + 1);
            sem_post(sem_parent);
        }
        sem_close(sem_parent);
        sem_close(sem_child);
        exit(EXIT_SUCCESS);
    } else {
        //Родительский процесс
        for (int i = 0; i < 10; i++) {
            sem_wait(sem_parent);
            printf("Родительский процесс: %d\n", i + 1);
            sem_post(sem_child);
        }
        waitpid(pid, NULL, 0);
        sem_close(sem_parent);
        sem_close(sem_child);
	//Удаляем семафоры
        sem_unlink(SEM_PARENT);
        sem_unlink(SEM_CHILD);
    }

    return 0;
}
