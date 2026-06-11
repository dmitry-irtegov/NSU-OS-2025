#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t parent_sem;
sem_t child_sem;

void *child(void *arg) {
    for (int i = 0; i < 11; i++) {
        sem_wait(&child_sem);

        printf("child thread %d\n", i);

        sem_post(&parent_sem);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    sem_init(&parent_sem, 0, 1);
    sem_init(&child_sem, 0, 0);

    pthread_create(&thread, NULL, child, NULL);

    for (int i = 0; i < 11; i++) {
        sem_wait(&parent_sem);

        printf("parent thread %d\n", i);

        sem_post(&child_sem);
    }

    pthread_join(thread, NULL);

    sem_destroy(&parent_sem);
    sem_destroy(&child_sem);

    return 0;
}