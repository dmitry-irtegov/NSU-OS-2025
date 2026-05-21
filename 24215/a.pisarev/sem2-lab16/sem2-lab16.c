#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SEM_PARENT_NAME "/sem_parent_seq"
#define SEM_CHILD_NAME  "/sem_child_seq"

int main() {
    sem_unlink(SEM_PARENT_NAME);
    sem_unlink(SEM_CHILD_NAME);
    sem_t *sem_parent = sem_open(SEM_PARENT_NAME, O_CREAT, 0644, 1);
    if (sem_parent == SEM_FAILED) { 
		perror("sem_open parent"); 
		exit(1); 
	}

    sem_t *sem_child = sem_open(SEM_CHILD_NAME, O_CREAT, 0644, 0);
    if (sem_child == SEM_FAILED) { 
		perror("sem_open child"); 
		sem_unlink(SEM_PARENT_NAME);
		exit(1); 
	}

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
		sem_unlink(SEM_PARENT_NAME);
		sem_unlink(SEM_CHILD_NAME);
        exit(1);
    }

    if (pid == 0) {
        for (int i = 0; i < 10; i++) {
            sem_wait(sem_child);
            printf("Current number is:%d (Child)\n", i);
            sem_post(sem_parent);
        }
        exit(0);
    } else {
        for (int i = 0; i < 10; i++) {
            sem_wait(sem_parent);
            printf("Current number is:%d (Parent)\n", i);
            sem_post(sem_child);
        }
        wait(NULL);

        sem_close(sem_parent);
        sem_close(sem_child);
        sem_unlink(SEM_PARENT_NAME);
        sem_unlink(SEM_CHILD_NAME);
    }

    return 0;
}
