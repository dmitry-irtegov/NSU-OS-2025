#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

#define ITERATIONS 10
#define MUTEX_COUNT 3
#define MUTEX_PARENT_START 0
#define MUTEX_CHILD_START 2

pthread_mutex_t mutex[MUTEX_COUNT];

void check_error(int errnum, const char *context) {
    if (errnum != 0) {
        fprintf(stderr, "%s: %s\n", context, strerror(errnum));
        exit(1);
    }
}

void *thread_function(void *arg) {
    check_error(pthread_mutex_lock(&mutex[MUTEX_CHILD_START]), "child: initial mutex lock error");

    for (int i = 0; i < ITERATIONS; i++) {
        int lock_idx = i % MUTEX_COUNT;
        int unlock_idx = (i + MUTEX_COUNT - 1) % MUTEX_COUNT;

        check_error(pthread_mutex_lock(&mutex[lock_idx]), "child: mutex lock error");
        printf("child %d\n", i + 1);
        check_error(pthread_mutex_unlock(&mutex[unlock_idx]), "child: mutex unlock error");
    }

    check_error(pthread_mutex_unlock(&mutex[(ITERATIONS + MUTEX_COUNT - 1) % MUTEX_COUNT]), "child: final mutex unlock error");
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

    check_error(pthread_mutex_lock(&mutex[MUTEX_PARENT_START]), "main: initial mutex lock error");
    check_error(pthread_create(&thread, NULL, thread_function, NULL), "error while creating thread");

    for (;;) {
        errnum = pthread_mutex_trylock(&mutex[MUTEX_CHILD_START]);
        if (errnum == EBUSY) {
            break;
        }
        if (errnum == 0) {
            check_error(pthread_mutex_unlock(&mutex[MUTEX_CHILD_START]), "main: startup mutex unlock error");
            sched_yield();
            continue;
        }
        check_error(errnum, "main: startup mutex trylock error");
    }

    for (int i = 0; i < ITERATIONS; i++) {
        int lock_idx = (i + 1) % MUTEX_COUNT;
        int unlock_idx = i % MUTEX_COUNT;

        check_error(pthread_mutex_lock(&mutex[lock_idx]), "parent: mutex lock error");
        printf("parent %d\n", i + 1);
        check_error(pthread_mutex_unlock(&mutex[unlock_idx]), "parent: mutex unlock error");
    }

    check_error(pthread_mutex_unlock(&mutex[ITERATIONS % MUTEX_COUNT]), "main: final mutex unlock error");
    check_error(pthread_join(thread, NULL), "pthread_join error");

    for (int i = 0; i < MUTEX_COUNT; i++) {
        check_error(pthread_mutex_destroy(&mutex[i]), "mutex_destroy error");
    }

    return 0;
}
