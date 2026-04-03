#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int turn = 0;

void* thread_func(void* arg) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);
        while (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }
        fprintf(stderr, "Child: %d\n", i + 1);
        turn = 0;
    
        pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    
    if (pthread_create(&thread, NULL, thread_func, NULL)) {
        fprintf(stderr, "Thread creation error\n");
        return 1;
    }
    
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);
        while (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        fprintf(stderr, "Parent: %d\n", i + 1);
        turn = 1;
 
        pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
    }
    
    if (pthread_join(thread, NULL)) {
        fprintf(stderr, "Thread join error\n");
        return 1;
    }
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}

