#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void cleanup_handler(void *arg) {
    printf("child thread is terminating\n");
}

void *foo(void* arg) {

    pthread_cleanup_push(cleanup_handler, NULL);

    while (1) {
        printf("child thread lines\n");
        sleep(1);
    }

    pthread_cleanup_pop(0);

    return NULL;
}

int main() {
    pthread_t thread;

    if (pthread_create(&thread, NULL, foo, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    sleep(3);

    if (pthread_cancel(thread) != 0) {
        perror("pthread_cancel");
        return 1;
    }

    pthread_join(thread, NULL);

    return 0;
}