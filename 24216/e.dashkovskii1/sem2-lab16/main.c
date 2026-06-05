#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

#define LINES 10
#define SEM_PARENT "/lab16_parent"
#define SEM_CHILD "/lab16_child"

static void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    sem_t *parent_turn, *child_turn;
    pid_t pid;
    int i;

    sem_unlink(SEM_PARENT);
    sem_unlink(SEM_CHILD);

    parent_turn = sem_open(SEM_PARENT, O_CREAT | O_EXCL, 0600, 1);
    if (parent_turn == SEM_FAILED) {
        handle_error("sem_open parent");
    }

    child_turn = sem_open(SEM_CHILD, O_CREAT | O_EXCL, 0600, 0);
    if (child_turn == SEM_FAILED) {
        handle_error("sem_open child");
    }

    pid = fork();
    if (pid < 0) {
        handle_error("fork");
    }

    if (pid == 0) {
        for (i = 0; i < LINES; i++) {
            if (sem_wait(child_turn) == -1) {
                handle_error("sem_wait child");
            }
            printf("thread\n");
            fflush(stdout);
            if (sem_post(parent_turn) == -1) {
                handle_error("sem_post parent");
            }
        }
        sem_close(child_turn);
        sem_close(parent_turn);
        exit(EXIT_SUCCESS);
    }

    for (i = 0; i < LINES; i++) {
        if (sem_wait(parent_turn) == -1) {
            handle_error("sem_wait parent");
        }
        printf("parent\n");
        fflush(stdout);
        if (sem_post(child_turn) == -1) {
            handle_error("sem_post child");
        }
    }

    wait(NULL);

    sem_close(parent_turn);
    sem_close(child_turn);
    sem_unlink(SEM_PARENT);
    sem_unlink(SEM_CHILD);
    return 0;
}
