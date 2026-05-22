#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void endline() {
    printf("\nThe thread finishes its work\n");
}

void* line() {

    pthread_cleanup_push(endline, NULL);

    int i = 0;
    for (;;i++) {
        printf("This is line number %d\n", i);
    }

    pthread_cleanup_pop(0);

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
    pthread_join(thread, NULL);

    printf("Thread canceled\n");
    return 0;
}
