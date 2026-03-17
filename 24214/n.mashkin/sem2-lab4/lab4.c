#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *thread() {
    for (int i = 0; i < 100; i++) {
        printf("Child: %d\n", i);
        usleep(300000);
    }
}

int main() {
    pthread_t thr;

    int res;
    if ((res = pthread_create(&thr, NULL, thread, NULL)) != 0) {
        fprintf(stderr, "pthread_create: %s", strerror(res));
        return 1;
    }

    sleep(2);

    if ((res = pthread_cancel(thr)) != 0) {
        fprintf(stderr, "pthread_cancel: %s", strerror(res));
        return 1;
    }

    return 0;
}

