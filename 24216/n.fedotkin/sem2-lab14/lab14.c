#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

sem_t sem_main_turn;
sem_t sem_child_turn;

void* thread_routine(void* arg) {
    
    for (int i = 1; i <= 10; i++) {
        if (sem_wait(&sem_child_turn) != 0) {
            perror("Error child sem_wait:");
            pthread_exit(NULL);
        }

        printf("Child: job %d\n", i);

        if (sem_post(&sem_main_turn) != 0) {
            perror("Error child sem_post:");
            pthread_exit(NULL);
        }
    }
    return NULL;
}

int main(void) {
    pthread_t worker_thread;

    if (sem_init(&sem_main_turn, 0, 1) != 0) {
        perror("Error init sem_main_turn");
        return EXIT_FAILURE;
    }

    if (sem_init(&sem_child_turn, 0, 0) != 0) {
        perror("Error init sem_child_turn");
        sem_destroy(&sem_main_turn);
        return EXIT_FAILURE;
    }

    int res = pthread_create(&worker_thread, NULL, thread_routine, NULL);
    if (res != 0) {
        fprintf(stderr, "Error pthread_create: %s\n", strerror(res));
        sem_destroy(&sem_main_turn);
        sem_destroy(&sem_child_turn);
        return EXIT_FAILURE;
    }

    for (int i = 1; i <= 10; i++) {
        if (sem_wait(&sem_main_turn) != 0) {
            perror("Error main sem_wait");
            break;
        }

        printf("Main: job %d\n", i);

        if (sem_post(&sem_child_turn) != 0) {
            perror("Error main sem_post");
            break;
        }
    }

    if (pthread_join(worker_thread, NULL) != 0) {
        perror("Error pthread_join");
    }

    sem_destroy(&sem_main_turn);
    sem_destroy(&sem_child_turn);

    return EXIT_SUCCESS;
}