#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t mutex;
pthread_cond_t cond;

int flag = 0; //0 - parent, 1 - thread

void* foo_thread() {
    for (int i = 1; i < 11; i++) {

        pthread_mutex_lock(&mutex);

        while (flag != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("thread line: %d\n", i);
        
        flag = 0;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main() {

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_cond_init(&cond, NULL);

    pthread_t thread;

    if (pthread_create(&thread, NULL, foo_thread, NULL) != 0) {
        printf("Error with thread creating");
        return EXIT_FAILURE;
    }

    for (int i = 1; i < 11; i++) {

        pthread_mutex_lock(&mutex);

        while (flag != 0) {
        pthread_cond_wait(&cond, &mutex);
        }

        printf("parent line: %d\n", i);

        flag = 1;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }

    pthread_join(thread, NULL);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    return 0;
}