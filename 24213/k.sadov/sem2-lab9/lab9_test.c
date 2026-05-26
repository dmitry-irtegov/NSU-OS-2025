#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NTHREADS 3

typedef struct {
    int role;
    pthread_t target_tid;
    pthread_mutex_t *mtx;
    pthread_cond_t *cv;
    volatile int *ready_flag;
} TestParam;

pthread_barrier_t barrier;

void log_tid(const char *action) {
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "thread %lu: %s\n", (unsigned long)pthread_self(), action);
    if (n > 0 && n < (int)sizeof(buf)) {
        write(STDOUT_FILENO, buf, (size_t)n);
    }
}

void sigint_handler(int sig) {
    (void)sig;
    log_tid("signal handler executing");
    log_tid("returning from handler — must resume waiting at barrier");
}

void *worker(void *param) {
    TestParam *p = (TestParam *)param;

    if (p->role == 0) {
        pthread_mutex_lock(p->mtx);
        *p->ready_flag = 1;
        pthread_cond_signal(p->cv);
        pthread_mutex_unlock(p->mtx);
        log_tid("calling barrier_wait");
        int rc = pthread_barrier_wait(&barrier);
        if (rc == PTHREAD_BARRIER_SERIAL_THREAD || rc == 0) {
            log_tid("passed barrier");
        }
    } else if (p->role == 1) {
        sleep(5);
        log_tid("calling barrier_wait");
        int rc = pthread_barrier_wait(&barrier);
        if (rc == PTHREAD_BARRIER_SERIAL_THREAD || rc == 0) {
            log_tid("passed barrier");
        }
    } else {
        pthread_mutex_lock(p->mtx);
        while (!*p->ready_flag) {
            pthread_cond_wait(p->cv, p->mtx);
        }
        pthread_mutex_unlock(p->mtx);
        sleep(2);
        log_tid("sending SIGINT to thread");
        pthread_kill(p->target_tid, SIGINT);
    }
    return NULL;
}

int main() {
    struct sigaction sa;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
    volatile int ready = 0;
    pthread_t ids[NTHREADS];
    TestParam params[NTHREADS];
    int err;

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    err = pthread_barrier_init(&barrier, NULL, 2);
    if (err != 0) {
        fprintf(stderr, "pthread_barrier_init: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NTHREADS; i++) {
        params[i].role = i;
        params[i].target_tid = 0;
        params[i].mtx = &mtx;
        params[i].cv = &cv;
        params[i].ready_flag = &ready;
    }

    err = pthread_create(&ids[0], NULL, worker, &params[0]);
    if (err != 0) {
        fprintf(stderr, "pthread_create thread 0: %s\n", strerror(err));
        pthread_barrier_destroy(&barrier);
        exit(EXIT_FAILURE);
    }

    params[2].target_tid = ids[0];

    err = pthread_create(&ids[1], NULL, worker, &params[1]);
    if (err != 0) {
        fprintf(stderr, "pthread_create thread 1: %s\n", strerror(err));
        pthread_barrier_destroy(&barrier);
        exit(EXIT_FAILURE);
    }

    err = pthread_create(&ids[2], NULL, worker, &params[2]);
    if (err != 0) {
        fprintf(stderr, "pthread_create thread 2: %s\n", strerror(err));
        pthread_barrier_destroy(&barrier);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NTHREADS; i++) {
        err = pthread_join(ids[i], NULL);
        if (err != 0) {
            fprintf(stderr, "error joining thread %d: %s\n", i, strerror(err));
        }
    }

    pthread_barrier_destroy(&barrier);
    return EXIT_SUCCESS;
}