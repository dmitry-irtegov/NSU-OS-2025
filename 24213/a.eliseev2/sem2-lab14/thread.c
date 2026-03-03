#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITERATIONS 10
#define STRERROR_BUF_SIZE 256

static sem_t sem1;
static sem_t sem2;

void *thread_run(void *arg) {
    int error;
    char err_msg[STRERROR_BUF_SIZE] = "(error message too long)";

    for (int i = 0; i < ITERATIONS; i++) {
        if ((error = sem_wait(&sem2))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "child wait failed: %s\n", err_msg);
            exit(1);
        }
        printf("I'm alive (child)\n");
        if ((error = sem_post(&sem1))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "child post failed: %s\n", err_msg);
            exit(1);
        }
    }
    pthread_exit(NULL);
}

int main() {
    int error;
    char err_msg[STRERROR_BUF_SIZE] = "(error message too long)";

    if ((error = sem_init(&sem1, 0, 1))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not init semaphore: %s\n", err_msg);
        exit(1);
    }
    if ((error = sem_init(&sem2, 0, 0))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not init semaphore: %s\n", err_msg);
        exit(1);
    }

    pthread_t pthread;

    if ((error = pthread_create(&pthread, NULL, thread_run, NULL))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not create thread: %s\n", err_msg);
        exit(1);
    }

    for (int i = 0; i < ITERATIONS; i++) {
        if ((error = sem_wait(&sem1))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "parent wait failed: %s\n", err_msg);
            exit(1);
        }
        printf("I'm alive (parent)\n");
        if ((error = sem_post(&sem2))) {
            strerror_r(error, err_msg, sizeof(err_msg));
            fprintf(stderr, "parent post failed: %s\n", err_msg);
            exit(1);
        }
    }
    
    if ((error = pthread_join(pthread, NULL))) {
        strerror_r(error, err_msg, sizeof(err_msg));
        fprintf(stderr, "could not join thread: %s\n", err_msg);
        exit(1);
    }

    return 0;
}
