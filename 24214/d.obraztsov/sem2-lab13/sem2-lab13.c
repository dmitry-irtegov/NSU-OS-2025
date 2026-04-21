#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define LINES_COUNT 10


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


// 0 - Parent, 1 - Child
int turn = 0; 


void* thread_body(void* arg) {
    for (int i = 0; i < LINES_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        while (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Child thread: line %d\n", i + 1);
        turn = 0;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;

}


int main() {

    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_body, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }


    for (int i = 0; i < LINES_COUNT; i++) {
        pthread_mutex_lock(&mutex);

        while (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Parent thread: line %d\n", i + 1);

        turn = 1;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    pthread_join(thread, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return EXIT_SUCCESS;

}