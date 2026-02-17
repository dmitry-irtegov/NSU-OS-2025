#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define CYCLES 10

typedef struct {
    pthread_mutex_t barrier;
    pthread_mutex_t parent_mut;
    pthread_mutex_t child_mut;
} sync_primitive;

sync_primitive sp;

void* child_thread(void* unused) {
    pthread_mutex_lock(&sp.child_mut);

    for (int i = 1; i <= CYCLES; i++) {
        pthread_mutex_lock(&sp.parent_mut);
        pthread_mutex_unlock(&sp.child_mut);

        printf("Child: %d\n", i);

        pthread_mutex_lock(&sp.barrier);
        pthread_mutex_unlock(&sp.parent_mut);

        pthread_mutex_lock(&sp.child_mut);
        pthread_mutex_unlock(&sp.barrier);
    }

    pthread_mutex_unlock(&sp.child_mut);
    return NULL;
}

void destroyAll(sync_primitive* sp) {
    pthread_mutex_destroy(&sp->barrier);
    pthread_mutex_destroy(&sp->parent_mut);
    pthread_mutex_destroy(&sp->child_mut);
}

int main(int argc, char* argv[]) {
    pthread_t child;
    pthread_mutexattr_t attr;
    int rc;

    rc = pthread_mutexattr_init(&attr);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: attr init: %s\n", argv[0], errbuf);
        return 1;
    }

    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: attr settype: %s\n", argv[0], errbuf);
        pthread_mutexattr_destroy(&attr);
        return 1;
    }

    rc = pthread_mutex_init(&sp.barrier, &attr);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: barrier init: %s\n", argv[0], errbuf);
        pthread_mutexattr_destroy(&attr);
        return 1;
    }

    rc = pthread_mutex_init(&sp.parent_mut, &attr);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: parent_mut init: %s\n", argv[0], errbuf);
        pthread_mutex_destroy(&sp.barrier);
        pthread_mutexattr_destroy(&attr);
        return 1;
    }

    rc = pthread_mutex_init(&sp.child_mut, &attr);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: child_mut init: %s\n", argv[0], errbuf);
        pthread_mutex_destroy(&sp.parent_mut);
        pthread_mutex_destroy(&sp.barrier);
        pthread_mutexattr_destroy(&attr);
        return 1;
    }

    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&sp.parent_mut);

    rc = pthread_create(&child, NULL, child_thread, NULL);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: thread create: %s\n", argv[0], errbuf);
        pthread_mutex_unlock(&sp.child_mut);
        pthread_mutex_unlock(&sp.parent_mut);
        destroyAll(&sp);
        return 1;
    }

    usleep(800);

    for (int i = 1; i <= CYCLES; i++) {
        printf("Parent: %d\n", i);

        pthread_mutex_lock(&sp.barrier);
        pthread_mutex_unlock(&sp.parent_mut);

        pthread_mutex_lock(&sp.child_mut);
        pthread_mutex_unlock(&sp.barrier);

        pthread_mutex_lock(&sp.parent_mut);
        pthread_mutex_unlock(&sp.child_mut);
    }

    rc = pthread_join(child, NULL);
    if (rc != 0) {
        char errbuf[256];
        strerror_r(rc, errbuf, sizeof(errbuf));
        fprintf(stderr, "%s: thread join: %s\n", argv[0], errbuf);
        pthread_mutex_unlock(&sp.parent_mut);
        destroyAll(&sp);
        return 1;
    }

    destroyAll(&sp);

    return 0;
}
