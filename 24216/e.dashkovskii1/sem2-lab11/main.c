#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sched.h>
#include <stdatomic.h>

pthread_mutex_t m[3];
atomic_int child_ready = 0;

void handle_error(int en, const char* msg) {
    if (en != 0) {
        errno = en;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *print_lines(void *arg) {
    (void)arg;

    handle_error(pthread_mutex_lock(&m[2]), "child: initial lock m[2]");
    atomic_store(&child_ready, 1);

    for (int i = 0; i < 10; i++) {
        handle_error(pthread_mutex_lock(&m[i % 3]), "child: lock next");
        fprintf(stderr, "thread\n");
        handle_error(pthread_mutex_unlock(&m[(i + 2) % 3]), "child: unlock current");
    }

    handle_error(pthread_mutex_unlock(&m[9 % 3]), "child: final unlock");
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_mutexattr_t attr;

    handle_error(pthread_mutexattr_init(&attr), "attr_init");
    handle_error(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK), "attr_settype");

    for (int i = 0; i < 3; i++) {
        handle_error(pthread_mutex_init(&m[i], &attr), "init mutex");
    }

    handle_error(pthread_mutexattr_destroy(&attr), "attr_destroy");
    handle_error(pthread_mutex_lock(&m[0]), "main: initial lock m[0]");

    int result = pthread_create(&tid, NULL, &print_lines, NULL);
    handle_error(result, "pthread_create");

    while (!atomic_load(&child_ready)) {
        sched_yield();
    }

    for (int i = 0; i < 10; i++) {
        handle_error(pthread_mutex_lock(&m[(i + 1) % 3]), "main: lock next");
        fprintf(stderr, "parent\n");
        handle_error(pthread_mutex_unlock(&m[i % 3]), "main: unlock current");
    }

    handle_error(pthread_mutex_unlock(&m[10 % 3]), "main: final unlock");

    result = pthread_join(tid, NULL);
    handle_error(result, "pthread_join");

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&m[i]);
    }

    exit(EXIT_SUCCESS);
}