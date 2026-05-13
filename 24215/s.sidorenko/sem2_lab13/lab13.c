#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t m[3];
volatile int child_ready = 0; 

void* child_thread() {
    pthread_mutex_lock(&m[2]);
    child_ready = 1;
    for(int i = 0; i < 10; i++) {
        pthread_mutex_lock(&m[i % 3]);
        
        printf("child thread: %d\n", i + 1);
        
        pthread_mutex_unlock(&m[(i + 2) % 3]);
    }
    
    pthread_mutex_unlock(&m[0]);
    return NULL;
}

int main() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&m[0], &attr);
    pthread_mutex_init(&m[1], &attr);
    pthread_mutex_init(&m[2], &attr);

    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&m[0]);
    pthread_mutex_lock(&m[1]);

    pthread_t thread;
    pthread_create(&thread, NULL, child_thread, NULL);

    while (!child_ready) {}

    for(int i = 0; i < 10; i++) {
        printf("parent thread: %d\n", i + 1);
        
        pthread_mutex_unlock(&m[i % 3]);
        pthread_mutex_lock(&m[(i + 2) % 3]);
    }

    pthread_join(thread, NULL);

    pthread_mutex_unlock(&m[1]);
    pthread_mutex_unlock(&m[2]);

    pthread_mutex_destroy(&m[0]);
    pthread_mutex_destroy(&m[1]);
    pthread_mutex_destroy(&m[2]);

    return 0;
}