#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>


sem_t doughter;
sem_t parent;

void* printline() {
    int i = 1;
    while (i <= 10) {
        sem_wait(&doughter);
        printf("Doughter print line %d\n", i++);
        sem_post(&parent);
    }
}

int main() {

    pthread_t thread;

    sem_init(&doughter, 0, 0);
    sem_init(&parent, 0, 0);

    pthread_create(&thread, NULL, printline, NULL);
    
    int i = 1;

    while (i <= 10) {
        printf("Parent print line %d\n", i++);
        sem_post(&doughter);
        sem_wait(&parent);
    }
    
    pthread_join(thread, NULL);

    sem_destroy(&doughter);
    sem_destroy(&parent);

    return 0;
}
