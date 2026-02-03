#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void endofthread() {
    printf("thread is ended\n");
}

void* threadfunc(void* arg) {
    int s = 0;
    pthread_cleanup_push(endofthread, NULL);
    while (1) {
        printf("seconds have passed:  %d\n", s++);
        sleep(1);
    }
    pthread_cleanup_pop(0);
}

int main() {
    pthread_t thread;
    int code = pthread_create(&thread, NULL, threadfunc, NULL);
    if (code != 0) {
        fprintf(stderr, "creating thread: %s\n", strerror(code));
        exit(EXIT_FAILURE);
    }
    sleep(2);
    code = pthread_cancel(thread);
    if (code != 0) {
        fprintf(stderr, "thread cancel error: %s\n", strerror(code));
        exit(EXIT_FAILURE);
    }
    code = pthread_join(thread, NULL);
    if (code != 0) {
        fprintf(stderr, "thread join error: %s\n", strerror(code));
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
