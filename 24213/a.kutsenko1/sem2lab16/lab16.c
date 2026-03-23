#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#define ITERATIONS 10

const char *sem_parent_name = "/kuc_par_sem";
const char *sem_child_name = "/kuc_chld_sem";

int main() {
    pid_t pid;
    sem_t *parent_sem, *child_sem;
    int i;
    
    parent_sem = sem_open(sem_parent_name, O_CREAT|O_EXCL, 0644, 1);
    if (parent_sem == SEM_FAILED) {
        perror("sem_open parent_sem failed");
        exit(1);
    }
        
    child_sem = sem_open(sem_child_name, O_CREAT|O_EXCL, 0644, 0);
    if (child_sem == SEM_FAILED) {
        perror("sem_open child_sem failed");
        sem_unlink(sem_parent_name);
        exit(1);
    }

    pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    
    if (pid == 0) {
        parent_sem = sem_open(sem_parent_name, O_CREAT, 0644, 1);
        if (parent_sem == SEM_FAILED) {
            perror("sem_open parent_sem failed");
            exit(1);
        }
        
        child_sem = sem_open(sem_child_name, O_CREAT, 0644, 0);
        if (child_sem == SEM_FAILED) {
            perror("sem_open child_sem failed");
            sem_unlink(sem_parent_name);
            exit(1);
        }

        for (i = 1; i <= ITERATIONS; i++) {
            if (sem_wait(child_sem) == -1) {
                perror("sem_wait child_sem failed");
                exit(1);
            }
            printf("Child: line %d\n", i);
            if (sem_post(parent_sem) == -1) {
                perror("sem_post parent_sem failed");
                exit(1);
            }
        }
        exit(0);
    } else {
        for (i = 1; i <= ITERATIONS; i++) {
            if (sem_wait(parent_sem) == -1) {
                perror("sem_wait parent_sem failed");
                exit(1);
            }
            printf("Parent: line %d\n", i);
            if (sem_post(child_sem) == -1) {
                perror("sem_post child_sem failed");
                exit(1);
            }
        }
        
        if (wait(NULL) == -1) {
            perror("wait fail");
            exit(1);
        }
        
        if (sem_unlink(sem_parent_name) == -1) {
            perror("sem_unlink parent_sem failed");
            sem_unlink(sem_child_name);
            exit(1);
        }

        if (sem_unlink(sem_child_name) == -1) {
            perror("sem_unlink child_sem failed");
            sem_unlink(sem_parent_name);
            exit(1);
        }
    }
    
    return 0;
}
