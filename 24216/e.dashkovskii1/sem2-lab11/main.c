#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sched.h>

pthread_mutex_t m_sync;
pthread_mutex_t m_parent;
pthread_mutex_t m_child;

volatile int child_ready = 0;

void handle_error(int en, const char* msg) {
    if (en != 0) {
        errno = en;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *print_lines(void *arg) {
    (void)arg;

    handle_error(pthread_mutex_lock(&m_child), "child: initial lock m_child");
    child_ready = 1;

    for (int i = 0; i < 10; i++) {
        handle_error(pthread_mutex_lock(&m_parent), "child: lock m_parent");
        handle_error(pthread_mutex_unlock(&m_child), "child: unlock m_child");

        fprintf(stderr, "thread\n");

        handle_error(pthread_mutex_lock(&m_sync), "child: lock m_sync");
        handle_error(pthread_mutex_unlock(&m_parent), "child: unlock m_parent");

        handle_error(pthread_mutex_lock(&m_child), "child: lock m_child");
        handle_error(pthread_mutex_unlock(&m_sync), "child: unlock m_sync");
    }

    handle_error(pthread_mutex_unlock(&m_child), "child: final unlock");
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_mutexattr_t attr;

    handle_error(pthread_mutexattr_init(&attr), "attr_init");
    handle_error(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK), "attr_settype");
    
    handle_error(pthread_mutex_init(&m_sync, &attr), "init m_sync");
    handle_error(pthread_mutex_init(&m_parent, &attr), "init m_parent");
    handle_error(pthread_mutex_init(&m_child, &attr), "init m_child");
    
    handle_error(pthread_mutexattr_destroy(&attr), "attr_destroy");

    handle_error(pthread_mutex_lock(&m_parent), "main: initial lock m_parent");

    int result = pthread_create(&tid, NULL, &print_lines, NULL);
    handle_error(result, "pthread_create");

    while (!child_ready) {
        sched_yield();
    }

    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "parent\n");

        handle_error(pthread_mutex_lock(&m_sync), "main: lock m_sync");
        handle_error(pthread_mutex_unlock(&m_parent), "main: unlock m_parent");

        handle_error(pthread_mutex_lock(&m_child), "main: lock m_child");
        handle_error(pthread_mutex_unlock(&m_sync), "main: unlock m_sync");

        handle_error(pthread_mutex_lock(&m_parent), "main: lock m_parent");
        handle_error(pthread_mutex_unlock(&m_child), "main: unlock m_child");
    }

    handle_error(pthread_mutex_unlock(&m_parent), "main: final unlock");

    result = pthread_join(tid, NULL);
    handle_error(result, "pthread_join");

    pthread_mutex_destroy(&m_child);
    pthread_mutex_destroy(&m_parent);
    pthread_mutex_destroy(&m_sync);

    exit(EXIT_SUCCESS);
}