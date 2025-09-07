#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t sem_parent;
sem_t sem_child;

void* child_thread() {
    for(int i = 0; i < 10; i++) {
        sem_wait(&sem_child);
        
        printf("Новая нить: строка %d\n", i + 1);
        
        sem_post(&sem_parent);
    }
    
    return NULL;
}

int main() {
    sem_init(&sem_parent, 0, 1);
    sem_init(&sem_child, 0, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, child_thread, NULL);

    for(int i = 0; i < 10; i++) {
        sem_wait(&sem_parent);
        
        printf("Родительская нить: строка %d\n", i + 1);
        
        sem_post(&sem_child);
    }

    pthread_join(thread, NULL);

    sem_destroy(&sem_parent);
    sem_destroy(&sem_child);

    return 0;
}