#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t sync_mutex;
pthread_cond_t  turn_cv;
int is_parent_turn = 1;

void* child_routine(void* arg) {
    int res;

    for (int i = 1; i <= 10; i++) {
        if ((res = pthread_mutex_lock(&sync_mutex)) != 0) {
            fprintf(stderr, "Error child lock: %s\n", strerror(res));
            exit(EXIT_FAILURE);
        }

        while (is_parent_turn) {
            pthread_cond_wait(&turn_cv, &sync_mutex);
        }

        printf("Child: job %d\n", i);

        is_parent_turn = 1;
        
        pthread_cond_signal(&turn_cv);

        if ((res = pthread_mutex_unlock(&sync_mutex)) != 0) {
            fprintf(stderr, "Error child unlock: %s\n", strerror(res));
            exit(EXIT_FAILURE);
        }
    }

    return NULL;
}

int main() {
    pthread_t tid;
    pthread_mutexattr_t m_attr;
    int res;

    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_ERRORCHECK);

    if ((res = pthread_mutex_init(&sync_mutex, &m_attr)) != 0) {
        fprintf(stderr, "Error mutex init: %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }

    if ((res = pthread_cond_init(&turn_cv, NULL)) != 0) {
        fprintf(stderr, "Error cond init: %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }

    if ((res = pthread_create(&tid, NULL, child_routine, NULL)) != 0) {
        fprintf(stderr, "Error thread create: %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 10; i++) {
        if ((res = pthread_mutex_lock(&sync_mutex)) != 0) {
            fprintf(stderr, "Error parent lock: %s\n", strerror(res));
            exit(EXIT_FAILURE);
        }

        while (!is_parent_turn) {
            pthread_cond_wait(&turn_cv, &sync_mutex);
        }

        printf("Main: job %d\n", i);

        is_parent_turn = 0;

        pthread_cond_signal(&turn_cv);

        if ((res = pthread_mutex_unlock(&sync_mutex)) != 0) {
            fprintf(stderr, "Error parent unlock: %s\n", strerror(res));
            exit(EXIT_FAILURE);
        }
    }

    pthread_join(tid, NULL);
    
    pthread_mutex_destroy(&sync_mutex);
    pthread_cond_destroy(&turn_cv);
    pthread_mutexattr_destroy(&m_attr);

    return EXIT_SUCCESS;
}