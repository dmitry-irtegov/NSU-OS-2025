#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

pthread_mutex_t mutex_midterm;
pthread_mutex_t mutex_child;
pthread_mutex_t mutex_parent;


void* ten_function(void* arg) {
    pthread_mutex_lock(&mutex_child);
    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(&mutex_parent);
        pthread_mutex_unlock(&mutex_child);
        printf("Child thread: line %d\n", i);
        pthread_mutex_lock(&mutex_midterm);
        pthread_mutex_unlock(&mutex_parent);
        pthread_mutex_lock(&mutex_child);
        pthread_mutex_unlock(&mutex_midterm);
    }
    pthread_mutex_unlock(&mutex_child);
    return 0;
}

int main() {
    pthread_t thread;

    pthread_mutexattr_t attr;
    
    int err = pthread_mutexattr_init(&attr);
    if (err != 0) {
        fprintf(stderr, "Mutexattr init error: %s\n", strerror(err));
        exit(1);
    }

    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0) {
        fprintf(stderr, "Mutex settype error: %s\n", strerror(err));
        exit(1);
    }

    err = pthread_mutex_init(&mutex_parent, &attr);
    if (err != 0) {
        fprintf(stderr, "Parent mutex init error: %s\n", strerror(err));
        exit(1);
    }

    err = pthread_mutex_init(&mutex_child, &attr);
    if (err != 0) {
        fprintf(stderr, "Child mutex init error: %s\n", strerror(err));
        exit(1);
    }

    err = pthread_mutex_init(&mutex_midterm, &attr);
    if (err != 0) {
        fprintf(stderr, "Midterm mutex init error: %s\n", strerror(err));
        exit(1);
    }

    pthread_mutex_lock(&mutex_parent);

    err = pthread_create(&thread, NULL, ten_function, NULL);
    if (err != 0) {
        fprintf(stderr, "Thread creation error: %s\n", strerror(err));
        pthread_mutex_unlock(&mutex_parent);
        exit(1);
    }

    usleep(1000);

    for (int i = 1; i <= 10; i++) {
        printf("Parent thread: line %d\n", i);
        pthread_mutex_lock(&mutex_midterm);
        pthread_mutex_unlock(&mutex_parent);
        pthread_mutex_lock(&mutex_child);
        pthread_mutex_unlock(&mutex_midterm);
        pthread_mutex_lock(&mutex_parent);
        pthread_mutex_unlock(&mutex_child);
    }

    pthread_mutex_unlock(&mutex_parent);

    err = pthread_join(thread, NULL);
    if (err != 0) {
        fprintf(stderr, "Thread join error: %s\n", strerror(err));
        exit(1);
    }
    return 0;
}

