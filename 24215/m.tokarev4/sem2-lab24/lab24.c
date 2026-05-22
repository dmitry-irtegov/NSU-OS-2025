#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>


sem_t detailA;
sem_t detailB;
sem_t detailC;

void* lineA() {
    while (1) {
        sleep(1);

        sem_post(&detailA);
    }
}

void* lineB() {
    while (1) {
        sleep(2);

        sem_post(&detailB);
    }
}

void* lineC() {
    while (1) {
        sleep(3);

        sem_post(&detailC);
    }
}

int main() {

    pthread_t A, B, C;

    sem_init(&detailA, 0, 0);
    sem_init(&detailB, 0, 0);
    sem_init(&detailC, 0, 0);

    pthread_create(&A, NULL, lineA, NULL);
    pthread_create(&B, NULL, lineB, NULL);
    pthread_create(&C, NULL, lineC, NULL);

    int cnt = 1;

    while (cnt < 10) {
        sem_wait(&detailC);
        sem_wait(&detailB);
        sem_wait(&detailA);

        printf("Widget number %d created\n", cnt++);
    }

    pthread_cancel(A);
    pthread_cancel(B);
    pthread_cancel(C);

    pthread_join(A, NULL);
    pthread_join(B, NULL);
    pthread_join(C, NULL);

    sem_destroy(&detailA);
    sem_destroy(&detailB);
    sem_destroy(&detailC);

    return 0;
}
