#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINES 10

enum {
    PARENT_TURN = 0,
    CHILD_TURN = 1
};

static pthread_mutex_t mutex;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int turn = PARENT_TURN;

static void die_pthread(const char *func, int rc)
{
    fprintf(stderr, "%s failed: %s\n", func, strerror(rc));
    exit(1);
}

static void *worker(void *arg)
{
    int rc;

    (void)arg;

    for (int i = 1; i <= LINES; ++i) {
        rc = pthread_mutex_lock(&mutex);
        if (rc != 0) {
            die_pthread("pthread_mutex_lock", rc);
        }

        while (turn != CHILD_TURN) {
            rc = pthread_cond_wait(&cond, &mutex);
            //??
            if (rc != 0) {
                die_pthread("pthread_cond_wait", rc);
            }
        }

        printf("child : line %2d, tid=%lu\n", i, (unsigned long)pthread_self());
        fflush(stdout);

        turn = PARENT_TURN;

        rc = pthread_cond_signal(&cond);
        if (rc != 0) {
            die_pthread("pthread_cond_signal", rc);
        }

        rc = pthread_mutex_unlock(&mutex);
        if (rc != 0) {
            die_pthread("pthread_mutex_unlock", rc);
        }
    }

    return NULL;
}

int main(void)
{
    pthread_t tid;
    pthread_mutexattr_t attr;
    int rc;

    setvbuf(stdout, NULL, _IOLBF, 0);

    rc = pthread_mutexattr_init(&attr);
    if (rc != 0) {
        die_pthread("pthread_mutexattr_init", rc);
    }

    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (rc != 0) {
        die_pthread("pthread_mutexattr_settype", rc);
    }

    rc = pthread_mutex_init(&mutex, &attr);
    if (rc != 0) {
        die_pthread("pthread_mutex_init", rc);
    }

    rc = pthread_mutexattr_destroy(&attr);
    if (rc != 0) {
        die_pthread("pthread_mutexattr_destroy", rc);
    }

    rc = pthread_create(&tid, NULL, worker, NULL);
    if (rc != 0) {
        die_pthread("pthread_create", rc);
    }

    for (int i = 1; i <= LINES; ++i) {
        rc = pthread_mutex_lock(&mutex);
        if (rc != 0) {
            die_pthread("pthread_mutex_lock", rc);
        }

        while (turn != PARENT_TURN) {
            rc = pthread_cond_wait(&cond, &mutex);
            if (rc != 0) {
                die_pthread("pthread_cond_wait", rc);
            }
        }

        printf("parent: line %2d, tid=%lu\n", i, (unsigned long)pthread_self());
        fflush(stdout);

        turn = CHILD_TURN;

        rc = pthread_cond_signal(&cond);
        if (rc != 0) {
            die_pthread("pthread_cond_signal", rc);
        }

        rc = pthread_mutex_unlock(&mutex);
        if (rc != 0) {
            die_pthread("pthread_mutex_unlock", rc);
        }
    }

    rc = pthread_join(tid, NULL);
    if (rc != 0) {
        die_pthread("pthread_join", rc);
    }

    rc = pthread_cond_destroy(&cond);
    if (rc != 0) {
        die_pthread("pthread_cond_destroy", rc);
    }

    rc = pthread_mutex_destroy(&mutex);
    if (rc != 0) {
        die_pthread("pthread_mutex_destroy", rc);
    }

    return 0;
}