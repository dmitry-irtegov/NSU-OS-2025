#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem1;
sem_t sem2;

void* ten_strings() {
    for (int i = 1; i <= 10; i++) {
        sem_wait(&sem2);
        printf("String №%d. (child)\n", i);
        sem_post(&sem1);
    }
    return NULL;
}

int main() {
    sem_init(&sem1, 0, 1);
    sem_init(&sem2, 0, 0);

    pthread_t thread;
    int code;

    code = pthread_create(&thread, NULL, ten_strings, NULL);
    if (code != 0) {
        fprintf(stderr, "Error creating thread.\n");
        return 1;
    }

    for (int i = 1; i <= 10; i++) {
        sem_wait(&sem1);
        printf("String №%d. (parent)\n", i);
        sem_post(&sem2);
    }

    code = pthread_join(thread, NULL);
    if (code != 0) {
        fprintf(stderr, "Error joining thread.\n");
        return 1;
    }

    sem_destroy(&sem1);
    sem_destroy(&sem2);

    return 0;
}