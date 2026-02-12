#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define ITERATIONS 10

sem_t s_worker;
sem_t s_manager;

void* thread_printer(void* arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        sem_wait(&s_worker);
        fprintf(stderr, "Child: job %d\n", i + 1);
        sem_post(&s_manager);
    }
    return NULL;
}

int main() {
    pthread_t worker_id;
    int result;

    sem_init(&s_worker, 0, 0);
    sem_init(&s_manager, 0, 1);

    result = pthread_create(&worker_id, NULL, thread_printer, NULL);

    if (result != 0) {
        fprintf(stderr, "Error pthread_create: %s\n", strerror(result));
        return 1; 
    }

    for (int i = 0; i < ITERATIONS; i++) {
        sem_wait(&s_manager);
        fprintf(stderr, "Parent: job %d\n", i + 1);
        sem_post(&s_worker);
    }
    result = pthread_join(worker_id, NULL);
    if (result != 0) {
        fprintf(stderr, "Error pthread_join: %s\n", strerror(result));
        return 1; 
    }

    sem_destroy(&s_worker);
    sem_destroy(&s_manager);

    return 0;
}
