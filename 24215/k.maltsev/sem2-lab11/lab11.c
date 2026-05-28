#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LINES 10
#define MUTEX_COUNT 3

static pthread_mutex_t mutexes[MUTEX_COUNT];

static void die_pthread(const char *func, int rc)
{
    fprintf(stderr, "%s failed: %s\n", func, strerror(rc));
    exit(1);
}

static void lock_mutex(int index)
{
    int rc = pthread_mutex_lock(&mutexes[index]);
    if (rc != 0) {
        die_pthread("pthread_mutex_lock", rc);
    }
}

static void unlock_mutex(int index)
{
    int rc = pthread_mutex_unlock(&mutexes[index]);
    if (rc != 0) {
        die_pthread("pthread_mutex_unlock", rc);
    }
}

static void rotate_mutexes(int *wait, int *release)
{
    int old_wait = *wait;

    *wait = 3 - *wait - *release;
    *release = old_wait;
}

static void init_mutexes(void)
{
    pthread_mutexattr_t attr;
    int rc;

    rc = pthread_mutexattr_init(&attr);
    if (rc != 0) {
        die_pthread("pthread_mutexattr_init", rc);
    }

    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (rc != 0) {
        die_pthread("pthread_mutexattr_settype", rc);
    }

    for (int i = 0; i < MUTEX_COUNT; ++i) {
        rc = pthread_mutex_init(&mutexes[i], &attr);
        if (rc != 0) {
            die_pthread("pthread_mutex_init", rc);
        }
    }

    rc = pthread_mutexattr_destroy(&attr);
    if (rc != 0) {
        die_pthread("pthread_mutexattr_destroy", rc);
    }
}

static void destroy_mutexes(void)
{
    int rc;

    for (int i = 0; i < MUTEX_COUNT; ++i) {
        rc = pthread_mutex_destroy(&mutexes[i]);
        if (rc != 0) {
            die_pthread("pthread_mutex_destroy", rc);
        }
    }
}

static void wait_child_initialization(void)
{
    int rc;

    for (;;) {
        rc = pthread_mutex_trylock(&mutexes[2]);

        if (rc == EBUSY) {
            return;
        }

        if (rc != 0) {
            die_pthread("pthread_mutex_trylock", rc);
        }

        unlock_mutex(2);

        sched_yield();
    }
}

static void *worker(void *arg)
{
    int wait = 1;
    int release = 2;

    (void)arg;

    lock_mutex(release);

    for (int i = 1; i <= LINES; ++i) {
        lock_mutex(wait);

        printf("child : line %2d, tid=%lu\n", i, (unsigned long)pthread_self());
        fflush(stdout);

        unlock_mutex(release);

        rotate_mutexes(&wait, &release);
    }

    unlock_mutex(release);

    return NULL;
}

int main(void)
{
    pthread_t tid;
    int rc;
    int wait = 0;
    int release = 1;

    setvbuf(stdout, NULL, _IOLBF, 0);

    init_mutexes();

    lock_mutex(release);

    rc = pthread_create(&tid, NULL, worker, NULL);
    if (rc != 0) {
        die_pthread("pthread_create", rc);
    }

    wait_child_initialization();

    for (int i = 1; i <= LINES; ++i) {
        lock_mutex(wait);

        printf("parent: line %2d, tid=%lu\n", i, (unsigned long)pthread_self());
        fflush(stdout);

        unlock_mutex(release);

        rotate_mutexes(&wait, &release);
    }

    unlock_mutex(release);

    rc = pthread_join(tid, NULL);
    if (rc != 0) {
        die_pthread("pthread_join", rc);
    }

    destroy_mutexes();

    return 0;
}