#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* line() {
    int i = 0;
    for (;;i++) {
        printf("This is line number %d\n", i);
    }
    return NULL;
}

int main() {

    pthread_t thread;

    if (pthread_create(&thread, NULL, line, NULL) != 0) {
        fprintf(stderr, "Error creating thread");
        return 1;
    }

    sleep(2);

    pthread_cancel(thread);

    printf("Thread canceled\n");
    return 0;
}
