#include <pthread.h>
#include <stdio.h>
#include <string.h>

void *thread() {
    for (int i = 0; i < 10; i++) {
        printf("Child: %d\n", i);
    }
}

int main() {
    pthread_t thr;

    int res;
    if ((res = pthread_create(&thr, NULL, thread, NULL)) != 0) {
        fprintf(stderr, "pthread_create: %s", strerror(res));
        return 1;
    }

    if ((res = pthread_join(thr, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s", strerror(res));
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("Parent: %d\n", i);
    }

    return 0;
}
