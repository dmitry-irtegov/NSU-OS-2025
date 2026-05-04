#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void child_cleanup(void *arg)
{
    fprintf(stderr, "Дочерняя нить завершает работу (cleanup)\n");
}

static void *child_thread()
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    int counter = 0;

    pthread_cleanup_push(child_cleanup, NULL);
    while (1) {
        fprintf(stderr, "Дочерняя нить работает... итерация %d\n", ++counter);
        sleep(1);
    }
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid;
    int err;

    if ((err = pthread_create(&tid, NULL, child_thread, NULL)) != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Дочерняя нить создана\n");

    sleep(2);

    fprintf(stderr, "Завершение дочерней нити\n");

    if ((err = pthread_cancel(tid)) != 0) {
        fprintf(stderr, "pthread_cancel: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((err = pthread_join(tid, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    return 0;
}