#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printStrings() {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Thread: Iteration %d\n", i);
    }
}

void *thread_print(void *unused) {
    (void)unused;
    printStrings();
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_print, NULL);
    if (err != 0) {
        fprintf(stderr, "error create thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    err = pthread_join(thread, NULL);
    if (err != 0) {
        fprintf(stderr, "error join thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    printStrings();
}
