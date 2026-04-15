#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = 0;

void* thread_func() {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);

        if (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Child thread %d\n", i + 1);
        turn = 0;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main () {
    pthread_t thread;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);

        if (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Parent thread %d\n", i + 1);
        turn = 1;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}
