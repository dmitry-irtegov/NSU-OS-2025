#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem_child;
sem_t sem_parent;

void* printer(void* arg) {
    for (int k = 0; k < 10; k++) {
        sem_wait(&sem_child);
        printf("Child thread: step %d\n", k);
        sem_post(&sem_parent);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int status;

    sem_init(&sem_child, 0, 0);
    sem_init(&sem_parent, 0, 1);

    status = pthread_create(&thread, NULL, printer, NULL);
    if (status != 0) {
        perror("Error creating thread");
        exit(1);
    }

    for (int k = 0; k < 10; k++) {
        sem_wait(&sem_parent);
        printf("Main thread:  step %d\n", k);
        sem_post(&sem_child);
    }

    pthread_join(thread, NULL);

    sem_destroy(&sem_child);
    sem_destroy(&sem_parent);

    return 0;
}