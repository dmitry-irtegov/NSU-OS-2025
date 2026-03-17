#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

pthread_mutex_t state_mutex;
pthread_mutex_t parent_mutex;
pthread_mutex_t child_mutex;

int turn = 0; 

void* child_thread(void* arg) {
    int ret;

    for (int i = 1; i <= 10; i++) {

        if ((ret = pthread_mutex_lock(&child_mutex)) != 0) {
            fprintf(stderr, "child: lock child_mutex failed: %s\n", strerror(ret));
            return NULL;
        }

        while (1) {
            if ((ret = pthread_mutex_lock(&state_mutex)) != 0) {
                fprintf(stderr, "child: lock state_mutex failed: %s\n", strerror(ret));
                return NULL;
            }

            if (turn == 1) {
                pthread_mutex_unlock(&state_mutex);
                break;
            }

            pthread_mutex_unlock(&state_mutex);
        }

        printf("child thread: line %d\n", i);

        if ((ret = pthread_mutex_lock(&state_mutex)) != 0) {
            fprintf(stderr, "child: lock state_mutex failed: %s\n", strerror(ret));
            return NULL;
        }

        turn = 0;

        if ((ret = pthread_mutex_unlock(&state_mutex)) != 0) {
            fprintf(stderr, "child: unlock state_mutex failed: %s\n", strerror(ret));
            return NULL;
        }

        if ((ret = pthread_mutex_unlock(&child_mutex)) != 0) {
            fprintf(stderr, "child: unlock child_mutex failed: %s\n", strerror(ret));
            return NULL;
        }
    }

    return NULL;
}

int main() {
    pthread_t tid;
    int ret;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&state_mutex, &attr);
    pthread_mutex_init(&parent_mutex, &attr);
    pthread_mutex_init(&child_mutex, &attr);

    if ((ret = pthread_create(&tid, NULL, child_thread, NULL)) != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
        return 1;
    }

    for (int i = 1; i <= 10; i++) {

        if ((ret = pthread_mutex_lock(&parent_mutex)) != 0) {
            fprintf(stderr, "parent: lock parent_mutex failed: %s\n", strerror(ret));
            return 1;
        }

        while (1) {
            if ((ret = pthread_mutex_lock(&state_mutex)) != 0) {
                fprintf(stderr, "parent: lock state_mutex failed: %s\n", strerror(ret));
                return 1;
            }

            if (turn == 0) {
                pthread_mutex_unlock(&state_mutex);
                break;
            }

            pthread_mutex_unlock(&state_mutex);
        }

        printf("parent thread: line %d\n", i);

        if ((ret = pthread_mutex_lock(&state_mutex)) != 0) {
            fprintf(stderr, "parent: lock state_mutex failed: %s\n", strerror(ret));
            return 1;
        }

        turn = 1;

        if ((ret = pthread_mutex_unlock(&state_mutex)) != 0) {
            fprintf(stderr, "parent: unlock state_mutex failed: %s\n", strerror(ret));
            return 1;
        }

        if ((ret = pthread_mutex_unlock(&parent_mutex)) != 0) {
            fprintf(stderr, "parent: unlock parent_mutex failed: %s\n", strerror(ret));
            return 1;
        }
    }

    pthread_join(tid, NULL);

    pthread_mutex_destroy(&state_mutex);
    pthread_mutex_destroy(&parent_mutex);
    pthread_mutex_destroy(&child_mutex);
    pthread_mutexattr_destroy(&attr);

    return 0;
}