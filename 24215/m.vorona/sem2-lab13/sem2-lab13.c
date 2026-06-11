#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINES 10

typedef enum {
    PARENT_TURN,
    CHILD_TURN
} turn_t;

static pthread_mutex_t mutex;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static turn_t turn = PARENT_TURN;

void *thread_func(void *arg) {
    (void)arg;

    for (int i = 1; i <= LINES; i++) {
        int err = pthread_mutex_lock(&mutex);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
            return NULL;
        }

        while (turn != CHILD_TURN) {
            err = pthread_cond_wait(&cond, &mutex);
            if (err != 0) {
                fprintf(stderr, "pthread_cond_wait: %s\n", strerror(err));
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }

        printf("child  %d\n", i);

        turn = PARENT_TURN;

        err = pthread_cond_signal(&cond);
        if (err != 0) {
            fprintf(stderr, "pthread_cond_signal: %s\n", strerror(err));
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        err = pthread_mutex_unlock(&mutex);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
            return NULL;
        }
    }

    return NULL;
}

int main() {
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

    err = pthread_mutex_init(&mutex, &attr);
    if (err != 0) {
        fprintf(stderr, "pthread_mutex_init: %s\n", strerror(err));
        pthread_mutexattr_destroy(&attr);
        return EXIT_FAILURE;
    }

    pthread_mutexattr_destroy(&attr);

    err = pthread_create(&tid, NULL, thread_func, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    for (int i = 1; i <= LINES; i++) {
        err = pthread_mutex_lock(&mutex);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
            pthread_mutex_destroy(&mutex);
            return EXIT_FAILURE;
        }

        while (turn != PARENT_TURN) {
            err = pthread_cond_wait(&cond, &mutex);
            if (err != 0) {
                fprintf(stderr, "pthread_cond_wait: %s\n", strerror(err));
                pthread_mutex_unlock(&mutex);
                pthread_mutex_destroy(&mutex);
                return EXIT_FAILURE;
            }
        }

        printf("parent %d\n", i);

        turn = CHILD_TURN;

        err = pthread_cond_signal(&cond);
        if (err != 0) {
            fprintf(stderr, "pthread_cond_signal: %s\n", strerror(err));
            pthread_mutex_unlock(&mutex);
            pthread_mutex_destroy(&mutex);
            return EXIT_FAILURE;
        }

        err = pthread_mutex_unlock(&mutex);
        if (err != 0) {
            fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
            pthread_mutex_destroy(&mutex);
            return EXIT_FAILURE;
        }
    }

    err = pthread_join(tid, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    return EXIT_SUCCESS;
}