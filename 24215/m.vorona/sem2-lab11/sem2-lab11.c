#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sched.h>

#define LINES 10
#define MUTEX_COUNT 3

static pthread_mutex_t mutexes[MUTEX_COUNT];

void *thread_func(void *arg)
{
    (void)arg;
    int err;

    int keep = 0;
    int wait = 1;
    int release = 2;

    err = pthread_mutex_lock(&mutexes[release]);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
        return NULL;
    }

    for (int i = 1; i <= LINES; i++) {
        err = pthread_mutex_lock(&mutexes[wait]);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
            return NULL;
        }

        printf("child  %d\n", i);

        err = pthread_mutex_unlock(&mutexes[release]);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
            return NULL;
        }

        int old_release = release;
        release = wait;
        wait = keep;
        keep = old_release;
    }

    err = pthread_mutex_unlock(&mutexes[release]);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
        return NULL;
    }

    return NULL;
}

int main(void)
{
    pthread_t tid;
    pthread_mutexattr_t attr;
    int err;

    err = pthread_mutexattr_init(&attr);
    if (err != 0) {
        fprintf(stderr, "pthread_mutexattr_init: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0) {
        fprintf(stderr, "pthread_mutexattr_settype: %s\n", strerror(err));
        pthread_mutexattr_destroy(&attr);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < MUTEX_COUNT; i++) {
        err = pthread_mutex_init(&mutexes[i], &attr);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_init: %s\n", strerror(err));
            pthread_mutexattr_destroy(&attr);
            return EXIT_FAILURE;
        }
    }

    pthread_mutexattr_destroy(&attr);

    err = pthread_mutex_lock(&mutexes[0]);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_mutex_lock(&mutexes[1]);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_create(&tid, NULL, thread_func, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    while (1) {
        err = pthread_mutex_trylock(&mutexes[2]);

        if (err == EBUSY) {
            break;
        }

        if (err == 0) {
            pthread_mutex_unlock(&mutexes[2]);
            sched_yield();
            continue;
        }

        fprintf(stderr, "pthread_mutex_trylock: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    int keep = 0;
    int release = 1;
    int wait = 2;

    for (int i = 1; i <= LINES; i++) {
        printf("parent %d\n", i);

        err = pthread_mutex_unlock(&mutexes[release]);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
            return EXIT_FAILURE;
        }

        err = pthread_mutex_lock(&mutexes[wait]);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
            return EXIT_FAILURE;
        }

        int old_release = release;
        release = keep;
        keep = wait;
        wait = old_release;
    }

    err = pthread_mutex_unlock(&mutexes[keep]);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_mutex_unlock(&mutexes[release]);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_join(tid, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < MUTEX_COUNT; i++) {
        pthread_mutex_destroy(&mutexes[i]);
    }

    return EXIT_SUCCESS;
}