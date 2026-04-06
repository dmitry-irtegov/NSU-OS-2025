#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t m1, m2, m3;

void* thread_routine() {
    pthread_mutex_lock(&m3);

    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(&m1);
        
        printf("child-thread:  line %d\n", i);
        
        pthread_mutex_unlock(&m3);
        pthread_mutex_lock(&m2);
        pthread_mutex_unlock(&m1);
        pthread_mutex_unlock(&m2);
        pthread_mutex_lock(&m3);
    }
    pthread_mutex_unlock(&m3);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&m1, &attr);
    pthread_mutex_init(&m2, &attr);
    pthread_mutex_init(&m3, &attr);
    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);

    if (pthread_create(&thread, NULL, thread_routine, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    usleep(10000);

    for (int i = 1; i <= 10; i++) {
        printf("parent-thread: line %d\n", i);

        pthread_mutex_unlock(&m1);
        pthread_mutex_lock(&m3);
        pthread_mutex_unlock(&m2);
        pthread_mutex_lock(&m1);
        pthread_mutex_lock(&m2);
        pthread_mutex_unlock(&m3);
    }

    pthread_join(thread, NULL);

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    pthread_mutex_destroy(&m3);
    pthread_mutexattr_destroy(&attr);

    return 0;
}