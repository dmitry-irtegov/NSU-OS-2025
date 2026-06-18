#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define WIDGETS_COUNT 10

sem_t sem_A;
sem_t sem_B;
sem_t sem_C;
sem_t sem_module;

void *make_A(void *arg) {
    for (int i = 1; i <= WIDGETS_COUNT; i++) {
        sleep(1);
        printf("[A] %d\n", i);
        sem_post(&sem_A);
    }
    return NULL;
}

void *make_B(void *arg) {
    for (int i = 1; i <= WIDGETS_COUNT; i++) {
        sleep(2);
        printf("[B] %d\n", i);
        sem_post(&sem_B);
    }
    return NULL;
}

void *make_C(void *arg) {
    for (int i = 1; i <= WIDGETS_COUNT; i++) {
        sleep(3);
        printf("[C] %d\n", i);
        sem_post(&sem_C);
    }
    return NULL;
}

void *make_module(void *arg) {
    for (int i = 1; i <= WIDGETS_COUNT; i++) {
        sem_wait(&sem_A);
        sem_wait(&sem_B);

        printf("[MODULE] %d\n", i);

        sem_post(&sem_module);
    }
    return NULL;
}

void *make_widget(void *arg) {
    for (int i = 1; i <= WIDGETS_COUNT; i++) {
        sem_wait(&sem_module);
        sem_wait(&sem_C);

        printf("[WIDGET] %d\n\n", i);
    }
    return NULL;
}

int main(void) {
    pthread_t thread_A;
    pthread_t thread_B;
    pthread_t thread_C;
    pthread_t thread_module;
    pthread_t thread_widget;

    sem_init(&sem_A, 0, 0);
    sem_init(&sem_B, 0, 0);
    sem_init(&sem_C, 0, 0);
    sem_init(&sem_module, 0, 0);

    pthread_create(&thread_A, NULL, make_A, NULL);
    pthread_create(&thread_B, NULL, make_B, NULL);
    pthread_create(&thread_C, NULL, make_C, NULL);
    pthread_create(&thread_module, NULL, make_module, NULL);
    pthread_create(&thread_widget, NULL, make_widget, NULL);
    

    pthread_join(thread_A, NULL);
    pthread_join(thread_B, NULL);
    pthread_join(thread_C, NULL);
    pthread_join(thread_module, NULL);
    pthread_join(thread_widget, NULL);

    sem_destroy(&sem_A);
    sem_destroy(&sem_B);
    sem_destroy(&sem_C);
    sem_destroy(&sem_module);

    printf("END: %d\n", WIDGETS_COUNT);

    return 0;
}