#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *printStrings(void *nothing) {
    for (int i = 0; i < 10; i++) {
        printf("Thread: Iteration %d\n", i);
    }
    fflush(stdout);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    if (pthread_create(&thread, NULL, printStrings, NULL) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    printStrings(NULL);
    pthread_join(thread, NULL);
    exit(EXIT_SUCCESS);
}
