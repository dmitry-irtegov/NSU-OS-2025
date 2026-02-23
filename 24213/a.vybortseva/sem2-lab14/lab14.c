#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#define COUNT 10

sem_t sem_p;
sem_t sem_c;

void* child_thread(void* arg) {
    int code;
    for (int i = 1; i <= COUNT; i++) {
        if (code = sem_wait(&sem_c)) {
            perror("sem_wait perror");
            exit(1);
        }
        printf("child: %d\n", i);
        if (code = sem_post(&sem_p)) {
            perror("sem_post perror");
            exit(1);
        }
    }
    return NULL;
}

int main() {

    if (sem_init(&sem_p, 0, 1)) {
        perror("sem_init error");
        exit(1);
    }

    if (sem_init(&sem_c, 0, 0)) {
        sem_destroy(&sem_p);
        perror("sem_init error");
        exit(1);
    }

    pthread_t thread;
    int code;

    if (code = pthread_create(&thread, NULL, child_thread, NULL)) {
        fprintf(stderr, "pthread_create error: %s\n", strerror(code));
        sem_destroy(&sem_p);
        sem_destroy(&sem_c);
        exit(1);
    }

    for (int i = 1; i <= COUNT; i++) {
        if (sem_wait(&sem_p)) {
            perror("sem_wait perror");
            exit(1);
        }
        printf("parent: %d\n", i);
        if (sem_post(&sem_c)) {
            perror("sem_post error");
            exit(1);
        }
    }

    if (code = pthread_join(thread, NULL)) {
        fprintf(stderr, "pthread_join error: %s\n", strerror(code));
        sem_destroy(&sem_p);
        sem_destroy(&sem_c);
        exit(1);
    }

    sem_destroy(&sem_p);
    sem_destroy(&sem_c);

    exit(0);
}
