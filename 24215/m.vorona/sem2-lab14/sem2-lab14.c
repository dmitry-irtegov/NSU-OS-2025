#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINES 10

static sem_t parent_sem;
static sem_t child_sem;

void *thread_func(void *arg) {
    (void)arg;

    for (int i = 1; i <= LINES; i++) {
        if (sem_wait(&child_sem) != 0) {
            perror("sem_wait");
            return NULL;
        }

        printf("child  %d\n", i);

        if (sem_post(&parent_sem) != 0) {
            perror("sem_post");
            return NULL;
        }
    }

    return NULL;
}

int main() {
    pthread_t tid;
    int err;

    if (sem_init(&parent_sem, 0, 1) != 0) {
        perror("sem_init");
        return 1;
    }

    if (sem_init(&child_sem, 0, 0) != 0) {
        perror("sem_init");
        sem_destroy(&parent_sem);
        return 1;
    }

    err = pthread_create(&tid, NULL, thread_func, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        sem_destroy(&child_sem);
        sem_destroy(&parent_sem);
        return 1;
    }

    for (int i = 1; i <= LINES; i++) {
        if (sem_wait(&parent_sem) != 0) {
            perror("sem_wait");
            sem_destroy(&child_sem);
            sem_destroy(&parent_sem);
            return 1;
        }

        printf("parent %d\n", i);

        if (sem_post(&child_sem) != 0) {
            perror("sem_post");
            sem_destroy(&child_sem);
            sem_destroy(&parent_sem);
            return 1;
        }
    }

    err = pthread_join(tid, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        sem_destroy(&child_sem);
        sem_destroy(&parent_sem);
        return 1;
    }

    sem_destroy(&child_sem);
    sem_destroy(&parent_sem);

    return 0;
}