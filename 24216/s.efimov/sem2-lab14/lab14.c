#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

static sem_t sem_main;
static sem_t sem_thread;

void* thread_work(void* arg) {
    for (int i = 0; i < 10; i++) {
        if (sem_wait(&sem_thread) != 0) {
            perror("Failed to wait on sem_thread");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Im child\n");
        if (sem_post(&sem_main) != 0) {
            perror("Failed to post on sem_main");
            exit(EXIT_FAILURE);
        }
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;   

    if (sem_init(&sem_main, 0, 1) != 0) {
        perror("Failed to initialize sem_main");
        return EXIT_FAILURE;
    }

    if (sem_init(&sem_thread, 0, 0) != 0) {
        perror("Failed to initialize sem_thread");
        sem_destroy(&sem_main);
        return EXIT_FAILURE;
    }

    if (pthread_create(&thread, NULL, thread_work, NULL) != 0) {
        perror("Failed to create thread");
        sem_destroy(&sem_main);
        sem_destroy(&sem_thread);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 10; i++) {
        if (sem_wait(&sem_main) != 0) {
            perror("Failed to wait on sem_main");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr,"Im parent\n");
        if (sem_post(&sem_thread) != 0) {
            perror("Failed to post on sem_thread");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("Failed to join thread");
        sem_destroy(&sem_main);
        sem_destroy(&sem_thread);
        return EXIT_FAILURE;
    }

    sem_destroy(&sem_main);
    sem_destroy(&sem_thread);
    return EXIT_SUCCESS;
}