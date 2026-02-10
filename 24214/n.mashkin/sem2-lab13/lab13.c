#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_LINES 10

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = 0;
    
void* child() {
    for (int i = 0; i < NUM_LINES; i++) {
        pthread_mutex_lock(&mutex);

        while (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Child: %d\n", i);
        turn = 0;
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
    }
    
    return NULL;
}

int main() {
    pthread_t thread;
    
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "mutex initialization error\n");
        return 1;
    }
    if (pthread_cond_init(&cond, NULL) != 0) {
        fprintf(stderr, "condition variable initialization error\n");
        return 1;
    }
    
    if (pthread_create(&thread, NULL, child, NULL) != 0) {
        fprintf(stderr, "thread creation error");
        return 1;
    }

    for (int i = 0; i < NUM_LINES; i++) {
        pthread_mutex_lock(&mutex);

        while (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        printf("Parent: %d\n", i);
        turn = 1;
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
    }
    
    pthread_join(thread, NULL);
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
