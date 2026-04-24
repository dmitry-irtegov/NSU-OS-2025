#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t m[3];

void* thread_func(void* arg) {
    pthread_mutex_lock(&m[1]);
    pthread_mutex_lock(&m[2]);
    
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Child: %d\n", i + 1);
        
        pthread_mutex_unlock(&m[(i + 1) % 3]);
        pthread_mutex_lock(&m[i % 3]);
    }
    
    pthread_mutex_unlock(&m[0]);
    pthread_mutex_unlock(&m[2]);
    
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutexattr_t attr;
    
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    
    for (int i = 0; i < 3; i++) {
        if (pthread_mutex_init(&m[i], &attr)) {
            fprintf(stderr, "Mutex init error\n");
            return 1;
        }
    }
    
    pthread_mutex_lock(&m[0]);
    pthread_mutex_lock(&m[2]);
    
    if (pthread_create(&thread, NULL, thread_func, NULL)) {
        fprintf(stderr, "Thread creation error\n");
        return 1;
    }
    
    usleep(100000); 
    
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Parent: %d\n", i + 1);
        
        pthread_mutex_unlock(&m[(i + 2) % 3]);
        pthread_mutex_lock(&m[(i + 1) % 3]);
    }
    
    pthread_mutex_unlock(&m[0]);
    pthread_mutex_unlock(&m[1]);
    
    if (pthread_join(thread, NULL)) {
        fprintf(stderr, "Thread join error\n");
        return 1;
    }
    
    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&m[i]);
    }
    pthread_mutexattr_destroy(&attr);
    
    return 0;
}