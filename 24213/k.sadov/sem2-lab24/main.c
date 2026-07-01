#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define THREAD_COUNT 4

sem_t sem_A, sem_B, sem_C, sem_AB;

volatile sig_atomic_t running = 1;
long widget_count = 0;

void sigint_handler(int signo) {
    running = 0;
    sem_post(&sem_AB);
    sem_post(&sem_C);
}

void *workerA(void *arg) {
    while (1) {
        sleep(1);
        sem_post(&sem_A);
        printf("A done\n");
    }
}

void *workerB(void *arg) {
    while (1) {
        sleep(2);
        sem_post(&sem_B);
        printf("B done\n");
    }
}

void *workerAB(void *arg) {
    while (1) {
        sem_wait(&sem_A);
        sem_wait(&sem_B);
        sem_post(&sem_AB);
        printf("AB done\n");
    }
}

void *workerC(void *arg) {
    while (1) {
        sleep(3);
        sem_post(&sem_C);
        printf("C done\n");
    }
}

void *workerWidget() {
    while (running) {
        sem_wait(&sem_AB);
        if (!running) {
            break;
        }
        sem_wait(&sem_C);
        if (!running) {
            break;
        }
        widget_count++;
        printf("Widget done\n");
    }
    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    struct sigaction sa;
    int rc;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sem_init(&sem_A, 0, 0);
    sem_init(&sem_B, 0, 0);
    sem_init(&sem_C, 0, 0);
    sem_init(&sem_AB, 0, 0);

    rc = pthread_create(&threads[0], NULL, workerA, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create A: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    rc = pthread_create(&threads[1], NULL, workerB, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create B: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    rc = pthread_create(&threads[2], NULL, workerC, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create C: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    rc = pthread_create(&threads[3], NULL, workerAB, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create AB: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    workerWidget();

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_cancel(threads[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nTotal widgets: %ld\n", widget_count);

    sem_destroy(&sem_A);
    sem_destroy(&sem_B);
    sem_destroy(&sem_C);
    sem_destroy(&sem_AB);

    exit(EXIT_SUCCESS);
}
