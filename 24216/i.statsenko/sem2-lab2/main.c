#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printStrings(char *str) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Thread %s: Iteration %d\n", str, i);
    }
}

void *thread_print(void *data) {
    printStrings((char *)data);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int err = pthread_create(&thread, NULL, thread_print, "child");
    if (err != 0) {
        fprintf(stderr, "error create thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    err = pthread_join(thread, NULL);
    if (err != 0) {
        fprintf(stderr, "error join thread: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
    printStrings("parent");
    exit(EXIT_SUCCESS);
}