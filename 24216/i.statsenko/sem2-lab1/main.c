#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *printStrings(void *) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Thread: Iteration %d\n", i);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int err = pthread_create(&thread, NULL, printStrings, NULL);
    if (err != 0) {
        fprintf(stderr, "error create thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    printStrings(NULL);
}
