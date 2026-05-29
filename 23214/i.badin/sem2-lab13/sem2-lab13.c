#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define ITERATIONS 10
#define PARENT_TURN 0
#define CHILD_TURN 1

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = PARENT_TURN;

void check_error(int errnum, const char *context) {
    if (errnum != 0) {
        fprintf(stderr, "%s: %s\n", context, strerror(errnum));
        exit(1);
    }
}

void *thread_function(void *arg) {
    for (int i = 0; i < ITERATIONS; i++) {
        check_error(pthread_mutex_lock(&mutex), "child: mutex lock error");

        while (turn != CHILD_TURN) {
            check_error(pthread_cond_wait(&cond, &mutex), "child: cond_wait error");
        }

        printf("child %d\n", i + 1);
        turn = PARENT_TURN;

        check_error(pthread_cond_signal(&cond), "child: cond_signal error");
        check_error(pthread_mutex_unlock(&mutex), "child: mutex unlock error");
    }

    return NULL;
}

int main() {
    pthread_t thread;

    check_error(pthread_mutex_init(&mutex, NULL), "mutex_init error");
    check_error(pthread_cond_init(&cond, NULL), "cond_init error");

    check_error(pthread_create(&thread, NULL, thread_function, NULL), "pthread_create error");

    for (int i = 0; i < ITERATIONS; i++) {
        check_error(pthread_mutex_lock(&mutex), "parent: mutex lock error");

        while (turn != PARENT_TURN) {
            check_error(pthread_cond_wait(&cond, &mutex), "parent: cond_wait error");
        }

        printf("parent %d\n", i + 1);
        turn = CHILD_TURN;

        check_error(pthread_cond_signal(&cond), "parent: cond_signal error");
        check_error(pthread_mutex_unlock(&mutex), "parent: mutex unlock error");
    }

    check_error(pthread_join(thread, NULL), "pthread_join error");
    check_error(pthread_cond_destroy(&cond), "cond_destroy error");
    check_error(pthread_mutex_destroy(&mutex), "mutex_destroy error");

    return 0;
}
