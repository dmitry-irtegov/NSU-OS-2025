#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *thread_func(void *arg)
{
    (void)arg;

    while (1) {
        printf("child is working\n");
        sleep(1);
    }

    return NULL;
}

int main(void)
{
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, thread_func, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        return 1;
    }

    sleep(2);

    err = pthread_cancel(tid);
    if (err != 0) {
        fprintf(stderr, "pthread_cancel: %s\n", strerror(err));
        return 1;
    }

    err = pthread_join(tid, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        return 1;
    }

    printf("child thread canceled\n");
    return 0;
}
