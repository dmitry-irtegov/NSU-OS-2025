#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define PARENT_THREAD 0
#define CHILD_THREAD 1

sem_t sem_parent; 
sem_t sem_child;

void *printTenRows(void *arg) {
    int id = *(int*)arg;

    if (id == PARENT_THREAD) {
        for (int i = 0; i < 10; i++) {
            sem_wait(&sem_parent);
            printf("Current number is:%d (%d)\n", i, id);
            sem_post(&sem_child);
        }
    } else if (id == CHILD_THREAD) {
        for (int i = 0; i < 10; i++) {
            sem_wait(&sem_child);
            printf("Current number is:%d (%d)\n", i, id);
            sem_post(&sem_parent);  
        }
    }
    return NULL;
}

int main() {
    if (sem_init(&sem_parent, 0, 1) != 0 || sem_init(&sem_child, 0, 0) != 0) {
        perror("Error sem_init");
        exit(1);
    }

    pthread_t child_tid;
    int child_id = CHILD_THREAD;

    if (pthread_create(&child_tid, NULL, printTenRows, &child_id) != 0) {
        perror("Error while creating thread");
        sem_destroy(&sem_parent);
        sem_destroy(&sem_child);
        exit(1);
    }

    int parent_id = PARENT_THREAD;
    printTenRows(&parent_id);
    pthread_join(child_tid, NULL);

    sem_destroy(&sem_parent);
    sem_destroy(&sem_child);
    return 0;
}
