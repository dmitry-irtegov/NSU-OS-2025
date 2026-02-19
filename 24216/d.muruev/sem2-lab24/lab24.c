#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


sem_t sem_a;
sem_t sem_b;
sem_t sem_c;
sem_t sem_module;

void* produce_a(void* arg) {
    while (1) {
        sleep(1);
        sem_post(&sem_a);
        fprintf(stderr, "Part A produced\n");
    }
    return NULL;
}

void* produce_b(void* arg) {
    while (1) {
        sleep(2);
        sem_post(&sem_b);
        fprintf(stderr, "Part A produced\n");
    }
    return NULL;
}


void* produce_c(void* arg) {
    while (1) {
        sleep(3);
        sem_post(&sem_c);
        fprintf(stderr, "Part C produced\n");
    }
    return NULL;
}


void* produce_module(void* arg) {
    while (1) {
        sem_wait(&sem_a);
        sem_wait(&sem_b);
        sem_post(&sem_module);
        fprintf(stderr, "Module produced (from A and B)\n");
    }
    return NULL;
}

void* produce_widget(void* arg) {
    while (1) {
        sem_wait(&sem_c);
        sem_wait(&sem_module);
        fprintf(stderr, "Widget produced (from Module and C)\n");
    }
    return NULL;
}

int main() {

    sem_init(&sem_a, 0, 0);
    sem_init(&sem_b, 0, 0);
    sem_init(&sem_c, 0, 0);
    sem_init(&sem_module, 0, 0);


    pthread_t thread_a, thread_b, thread_c, thread_mod, thread_wid;

    pthread_create(&thread_a, NULL, produce_a, NULL);
    pthread_create(&thread_b, NULL, produce_b, NULL);
    pthread_create(&thread_c, NULL, produce_c, NULL);
    pthread_create(&thread_mod, NULL, produce_module, NULL);
    pthread_create(&thread_wid, NULL, produce_widget, NULL);


    pthread_join(thread_wid, NULL);
    pthread_join(thread_mod, NULL);
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
    pthread_join(thread_c, NULL);

    return 0;
}
