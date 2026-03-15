#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t m0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;
volatile int ready = 0;

#define steps 10

void *worker(void *args) {
    pthread_mutex_lock(&m1);
    ready = 1;
    for (int i = 0; i < steps; i++) {

        if (i % 3 == 0) {
            pthread_mutex_lock(&m0);
            printf("Child: %d\n", i + 1);
            pthread_mutex_unlock(&m1);
        } else if (i % 3 == 1) {
            pthread_mutex_lock(&m2);
            printf("Child: %d\n", i + 1);
            pthread_mutex_unlock(&m0);
        } else {
            pthread_mutex_lock(&m1);
            printf("Child: %d\n", i + 1);
            pthread_mutex_unlock(&m2);
        }
    }
    pthread_mutex_unlock(&m0);
    return NULL;
}

int main() {
    pthread_mutexattr_t attr;
    pthread_t ch;
    int ret;

    ret = pthread_mutexattr_init(&attr);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        fprintf(stderr, "pthread_mutexattr_init: %s\n", errBuf);
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        fprintf(stderr, "pthread_mutexattr_settype: %s\n", errBuf);
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutex_init(&m0, &attr);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        fprintf(stderr, "pthread_mutex_init(m0): %s\n", errBuf);
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutex_init(&m1, &attr);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        fprintf(stderr, "pthread_mutex_init(m1): %s\n", errBuf);
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }

    ret = pthread_mutex_init(&m2, &attr);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        fprintf(stderr, "pthread_mutex_init(m2): %s\n", errBuf);
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&m0);

    ret = pthread_create(&ch, NULL, worker, NULL);
    if (ret != 0) {
        char errBuf[256];
        strerror_r(ret, errBuf, sizeof(errBuf));
        fprintf(stderr, "pthread_create: %s\n", errBuf);
        exit(EXIT_FAILURE);
    }

    while (!ready) {
    }

    for (int i = 0; i < steps; i++) {
        if (i % 3 == 0) {
            pthread_mutex_lock(&m2);
            printf("Parent: %d\n", i + 1);
            pthread_mutex_unlock(&m0);
        } else if (i % 3 == 1) {
            pthread_mutex_lock(&m1);
            printf("Parent: %d\n", i + 1);
            pthread_mutex_unlock(&m2);
        } else {
            pthread_mutex_lock(&m0);
            printf("Parent: %d\n", i + 1);
            pthread_mutex_unlock(&m1);
        }
    }

    pthread_mutex_unlock(&m2);

    ret = pthread_join(ch, NULL);

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m0);
    pthread_mutex_destroy(&m2);

    exit(EXIT_SUCCESS);
}

