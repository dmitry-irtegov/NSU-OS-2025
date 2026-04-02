#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

#define ITERATIONS 10
#define MUTEX_COUNT 3
#define MUTEX_COMMON 0
#define MUTEX_PARENT 1
#define MUTEX_CHILD 2

pthread_mutex_t mutex[MUTEX_COUNT];

void check_error(int errnum, const char *context) {
    if (errnum != 0) {
        fprintf(stderr, "%s: %s\n", context, strerror(errnum));
        exit(1);
    }
}

void *thread_function(void *arg) {
    check_error(pthread_mutex_lock(&mutex[MUTEX_CHILD]), "child: mutex_child lock error");

    for (int i = 0; i < ITERATIONS; i++) {
        check_error(pthread_mutex_lock(&mutex[MUTEX_PARENT]), "child: mutex_parent lock error");
        printf("child %d\n", i + 1);
        check_error(pthread_mutex_unlock(&mutex[MUTEX_CHILD]), "child: mutex_child unlock error");

        check_error(pthread_mutex_lock(&mutex[MUTEX_COMMON]), "child: mutex_common lock error");
        check_error(pthread_mutex_unlock(&mutex[MUTEX_PARENT]), "child: mutex_parent unlock error");
        check_error(pthread_mutex_lock(&mutex[MUTEX_CHILD]), "child: mutex_child relock error");
        check_error(pthread_mutex_unlock(&mutex[MUTEX_COMMON]), "child: mutex_common unlock error");
    }

    check_error(pthread_mutex_unlock(&mutex[MUTEX_CHILD]), "child: final mutex_child unlock error");
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutexattr_t attr;
    int errnum;

    check_error(pthread_mutexattr_init(&attr), "mutexattr_init error");
    check_error(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK), "mutexattr_settype error");

    for (int i = 0; i < MUTEX_COUNT; i++) {
        check_error(pthread_mutex_init(&mutex[i], &attr), "mutex_init error");
    }
    pthread_mutexattr_destroy(&attr);

    check_error(pthread_mutex_lock(&mutex[MUTEX_PARENT]), "main: initial mutex_parent lock error");
    check_error(pthread_create(&thread, NULL, thread_function, NULL), "error while creating thread");

    for (;;) {
        errnum = pthread_mutex_trylock(&mutex[MUTEX_CHILD]);
        if (errnum == EBUSY) {
            break;
        }
        if (errnum == 0) {
            pthread_mutex_unlock(&mutex[MUTEX_CHILD]);
            sched_yield();
            continue;
        }
        check_error(errnum, "main: mutex_child trylock error");
    }

    for (int i = 0; i < ITERATIONS; i++) {
        printf("parent %d\n", i + 1);

        check_error(pthread_mutex_lock(&mutex[MUTEX_COMMON]), "parent: mutex_common lock error");
        check_error(pthread_mutex_unlock(&mutex[MUTEX_PARENT]), "parent: mutex_parent unlock error");
        check_error(pthread_mutex_lock(&mutex[MUTEX_CHILD]), "parent: mutex_child lock error");
        check_error(pthread_mutex_unlock(&mutex[MUTEX_COMMON]), "parent: mutex_common unlock error");
        check_error(pthread_mutex_lock(&mutex[MUTEX_PARENT]), "parent: mutex_parent relock error");
        check_error(pthread_mutex_unlock(&mutex[MUTEX_CHILD]), "parent: mutex_child unlock error");
    }

    check_error(pthread_mutex_unlock(&mutex[MUTEX_PARENT]), "main: final mutex_parent unlock error");
    check_error(pthread_join(thread, NULL), "pthread_join error");

    for (int i = 0; i < MUTEX_COUNT; i++) {
        check_error(pthread_mutex_destroy(&mutex[i]), "mutex_destroy error");
    }

    return 0;
}
