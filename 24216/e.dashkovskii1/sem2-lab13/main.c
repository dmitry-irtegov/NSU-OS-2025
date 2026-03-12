#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

pthread_mutex_t mtx;
pthread_cond_t cv;

int turn = 0;

void handle_error(int en, const char *msg) {
    if (en != 0) {
        errno = en;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *print_lines(void *arg) {
    (void)arg;

    for (int i = 0; i < 10; i++) {
        handle_error(pthread_mutex_lock(&mtx), "child: lock");

        while (turn == 0) {
            handle_error(pthread_cond_wait(&cv, &mtx), "child: cond_wait");
        }

        fprintf(stderr, "thread\n");

        turn = 0;

        handle_error(pthread_cond_signal(&cv), "child: signal");
        handle_error(pthread_mutex_unlock(&mtx), "child: unlock");
    }

    return NULL;
}

int main() {
    pthread_t tid;
    pthread_mutexattr_t attr;

    handle_error(pthread_mutexattr_init(&attr), "attr_init");
    handle_error(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK), "attr_settype");
    
    handle_error(pthread_mutex_init(&mtx, &attr), "init mtx");
    handle_error(pthread_mutexattr_destroy(&attr), "attr_destroy");
    handle_error(pthread_cond_init(&cv, NULL), "init cv");

    int result = pthread_create(&tid, NULL, &print_lines, NULL);
    handle_error(result, "pthread_create");

    for (int i = 0; i < 10; i++) {
        handle_error(pthread_mutex_lock(&mtx), "main: lock");

        while (turn == 1) {
            handle_error(pthread_cond_wait(&cv, &mtx), "main: cond_wait");
        }

        fprintf(stderr, "parent\n");

        turn = 1;

        handle_error(pthread_cond_signal(&cv), "main: signal");
        handle_error(pthread_mutex_unlock(&mtx), "main: unlock");
    }

    result = pthread_join(tid, NULL);
    handle_error(result, "pthread_join");

    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cv);

    exit(EXIT_SUCCESS);
}
