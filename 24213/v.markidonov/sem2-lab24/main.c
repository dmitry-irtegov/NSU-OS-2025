#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem_a, sem_b, sem_c, sem_module;

void handle_pthread_error(int code, char* msg) {
    if (code != 0) {
        fprintf(stderr, "%s: %s\n", msg, strerror(code));
        exit(1);
    }
}

void handle_error(int code, char* msg) {
    if (code) {
        perror(msg);
        exit(1);
    }
}

void* produce_A(void* arg) {
    while (1) {
        sleep(1);
        handle_error(sem_post(&sem_a), "sem post A");
        printf("produced A\n");
    }
}

void* produce_B(void* arg) {
    while (1) {
        sleep(2);
        handle_error(sem_post(&sem_b), "sem post B");
        printf("produced B\n");
    }
}

void* produce_C(void* arg) {
    while (1) {
        sleep(3);
        handle_error(sem_post(&sem_c), "sem post C");
        printf("produced C\n");
    }
}

void* produce_module(void* arg) {
    while (1) {
        handle_error(sem_wait(&sem_a), "sem wait A");
        handle_error(sem_wait(&sem_b), "sem wait B");
        handle_error(sem_post(&sem_module), "sem post module");
        printf("produced module\n");
    }
}

void* produce_widget() {
    while (1) {
        handle_error(sem_wait(&sem_c), "sem wait C");
        handle_error(sem_wait(&sem_module), "sem wait module");
        printf("produced widget\n");
    }
}

int main() {
    handle_error(sem_init(&sem_a, 0, 0), "sem init A");
    handle_error(sem_init(&sem_b, 0, 0), "sem init B");
    handle_error(sem_init(&sem_c, 0, 0), "sem init C");
    handle_error(sem_init(&sem_module, 0, 0), "sem init module");

    pthread_t pthread_a, pthread_b, pthread_c, pthread_module;

    handle_pthread_error(pthread_create(&pthread_a, NULL, produce_A, NULL), "create thread A");
    handle_pthread_error(pthread_create(&pthread_b, NULL, produce_B, NULL), "create thread B");
    handle_pthread_error(pthread_create(&pthread_c, NULL, produce_C, NULL), "create thread C");
    handle_pthread_error(pthread_create(&pthread_module, NULL, produce_module, NULL), "create thread D");

    produce_widget();

    return 0;
}
