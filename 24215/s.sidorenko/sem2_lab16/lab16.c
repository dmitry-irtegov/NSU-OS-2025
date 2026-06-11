#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(void)
{
    char sp_name[32], sc_name[32];
    snprintf(sp_name, sizeof(sp_name), "/sem_p_%d", getpid());
    snprintf(sc_name, sizeof(sc_name), "/sem_c_%d", getpid());

    sem_t *sp = sem_open(sp_name, O_CREAT | O_EXCL, 0600, 1);
    sem_t *sc = sem_open(sc_name, O_CREAT | O_EXCL, 0600, 0);

    if (sp == SEM_FAILED || sc == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        sem_unlink(sp_name);
        sem_unlink(sc_name);
        return 1;
    }

    if (pid == 0) {
        for (int i = 1; i <= 10; i++) {
            sem_wait(sc);
            printf("child proc %d\n", i);
            sem_post(sp);
        }
        return 0;
    }

    for (int i = 1; i <= 10; i++) {
        sem_wait(sp);
        printf("parent proc %d\n", i);
        sem_post(sc);
    }

    wait(NULL);
    sem_close(sc);
    sem_close(sp);
    sem_unlink(sc_name);
    sem_unlink(sp_name);
    return 0;
}