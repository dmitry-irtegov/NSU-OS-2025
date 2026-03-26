#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
#include <string.h>

pthread_mutex_t sync_mtx[3];
atomic_int thread_active = 0;

void check_status(int code, const char* msg) {
    if (code != 0) {
        fprintf(stderr, "Error %s. Message: %s\n", msg, strerror(code));
        exit(EXIT_FAILURE);
    }
}

void* thread_routine(void* arg) {

    check_status(pthread_mutex_lock(&sync_mtx[0]), "child pthread_mutex_lock");
    atomic_store(&thread_active, 1);

    for (int step = 0; step < 10; step++) {
        check_status(pthread_mutex_lock(&sync_mtx[(step + 1) % 3]), "child pthread_mutex_lock");
        printf("Child: job %d\n", step);
        
        check_status(pthread_mutex_unlock(&sync_mtx[step % 3]), "child pthread_mutex_unlock");
    }

    check_status(pthread_mutex_unlock(&sync_mtx[10 % 3]), "child pthread_mutex_unlock");
    return NULL;
}

int main(void) {
    pthread_mutexattr_t mtx_attr;
    check_status(pthread_mutexattr_init(&mtx_attr), "pthread_mutexattr_init");
    check_status(pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_ERRORCHECK), "pthread_mutexattr_settype");

    for (int k = 0; k < 3; k++) {
        check_status(pthread_mutex_init(&sync_mtx[k], &mtx_attr), "pthread_mutex_init");
    }
    
    check_status(pthread_mutexattr_destroy(&mtx_attr), "pthread_mutexattr_destroy");
    
    check_status(pthread_mutex_lock(&sync_mtx[1]), "main pthread_mutex_lock");

    pthread_t worker_thread;
    check_status(pthread_create(&worker_thread, NULL, thread_routine, NULL), "pthread_create");

    while (!atomic_load(&thread_active)) {
        sched_yield();
    }

    for (int iter = 0; iter < 10; iter++) {
        check_status(pthread_mutex_lock(&sync_mtx[(iter + 2) % 3]), "main pthread_mutex_lock");
        printf("Main: job %d\n", iter);
        
        check_status(pthread_mutex_unlock(&sync_mtx[(iter + 1) % 3]), "main pthread_mutex_unlock");
    }

    check_status(pthread_mutex_unlock(&sync_mtx[11 % 3]), "main pthread_mutex_unlock");

    check_status(pthread_join(worker_thread, NULL), "main pthread_join");

    for (int k = 0; k < 3; k++) {
        pthread_mutex_destroy(&sync_mtx[k]);
    }

    return EXIT_SUCCESS;
}