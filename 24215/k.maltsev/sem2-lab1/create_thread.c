#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *worker(void *arg)
{
    (void)arg;

    for (int i = 1; i <= 10; ++i) {
        printf("child : line %2d, tid=%lu\n", i, (unsigned long)pthread_self());
        fflush(stdout);
    }
    return NULL;
}

int main(void)
{
    pthread_t tid;
    int rc;
    setvbuf(stdout, NULL, _IOLBF, 0);

    rc = pthread_create(&tid, NULL, worker, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(rc));
        return 1;
    }

    for (int i = 1; i <= 10; ++i) {
        printf("parent: line %2d, tid=%lu\n", i, (unsigned long)pthread_self());
        fflush(stdout);
    }

    rc = pthread_join(tid, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
        return 1;
    }

    return 0;
}