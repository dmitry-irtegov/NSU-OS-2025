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
        printf("Part A produced\n");
        sem_post(&sem_a);
    }
    return NULL;
}

void* produce_b(void* arg) {
    while (1) {
        sleep(2);
        printf("Part B produced\n");
        sem_post(&sem_b);
    }
    return NULL;
}


void* produce_c(void* arg) {
    while (1) {
        sleep(3);
        printf("Part C produced\n");
        sem_post(&sem_c);
    }
    return NULL;
}


void* produce_module(void* arg) {
    while (1) {
        sem_wait(&sem_a);
        sem_wait(&sem_b);
        printf("Module produced (from A and B)\n");
        sem_post(&sem_module);
    }
    return NULL;
}

void* produce_widget(void* arg) {
    while (1) {
        sem_wait(&sem_c);
        sem_wait(&sem_module);
        printf("Widget produced (from Module and C)\n");
    }
    return NULL;
}

int main() {
    sem_init(&sem_a, 0, 0);
    sem_init(&sem_b, 0, 0);
    sem_init(&sem_c, 0, 0);
    sem_init(&sem_module, 0, 0);

    pthread_t thread_a, thread_b, thread_c, thread_mod, thread_wid;

    // Create threads
    if (pthread_create(&thread_a, NULL, produce_a, NULL) != 0) {
        perror("Failed to create thread A");
        return 1;
    }
    if (pthread_create(&thread_b, NULL, produce_b, NULL) != 0) {
        perror("Failed to create thread B");
        return 1;
    }
    if (pthread_create(&thread_c, NULL, produce_c, NULL) != 0) {
        perror("Failed to create thread C");
        return 1;
    }
    if (pthread_create(&thread_mod, NULL, produce_module, NULL) != 0) {
        perror("Failed to create thread Module");
        return 1;
    }
    if (pthread_create(&thread_wid, NULL, produce_widget, NULL) != 0) {
        perror("Failed to create thread Widget");
        return 1;
    }

    pthread_join(thread_wid, NULL);
    pthread_join(thread_mod, NULL);
    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
    pthread_join(thread_c, NULL);

    sem_destroy(&sem_a);
    sem_destroy(&sem_b);
    sem_destroy(&sem_c);
    sem_destroy(&sem_module);

    return 0;
}
