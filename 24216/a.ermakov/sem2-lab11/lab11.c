#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define NUM_LINES 10

pthread_mutex_t mutex;
pthread_cond_t parent_cond;
pthread_cond_t child_cond;
int turn = 0;

void* thread_routine(void* arg) {
    (void) arg;

    for (int i = 0; i < NUM_LINES; i++) {
        pthread_mutex_lock(&mutex);
        while (turn != 1) {
            pthread_cond_wait(&child_cond, &mutex);
        }
        
        fprintf(stderr, "Дочерняя нить: строка %d\n", i + 1);
        turn = 0;
        pthread_cond_signal(&parent_cond);
        
        pthread_mutex_unlock(&mutex);
    }
    
    return NULL;
}

int main() {
    pthread_t thr;
    int status;
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    
    pthread_mutex_init(&mutex, &attr);
    pthread_cond_init(&parent_cond, NULL);
    pthread_cond_init(&child_cond, NULL);
    pthread_mutexattr_destroy(&attr);

    turn = 0;

    status = pthread_create(&thr, NULL, thread_routine, NULL);
    if (status != 0) {
        fprintf(stderr, "Ошибка создания потока: %s\n", strerror(status));
        return 1;
    }

    for (int i = 0; i < NUM_LINES; i++) {
        pthread_mutex_lock(&mutex);
        while (turn != 0) {
            pthread_cond_wait(&parent_cond, &mutex);
        }
        
        fprintf(stderr, "Родительская нить: строка %d\n", i + 1);
        turn = 1;
        pthread_cond_signal(&child_cond);
        
        pthread_mutex_unlock(&mutex);
    }

    pthread_join(thr, NULL);
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&parent_cond);
    pthread_cond_destroy(&child_cond);

    return 0;
}
