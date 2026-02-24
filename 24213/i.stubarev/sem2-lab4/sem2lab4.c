#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *child_thread(void* p) {
    while (1)
    {
        printf("Child thread is running\n");
    }
}

int main() {
    pthread_t child;

    int rc = pthread_create(&child, NULL, child_thread, NULL);
    if (rc != 0) {
        fprintf(stderr, "Thread creating error: %s\n", strerror(rc));
        exit(1);
    }

    sleep(2);

    rc = pthread_cancel(child);
    if (rc != 0) {
        fprintf(stderr, "Thread cancel error: %s\n", strerror(rc));
        exit(1);
    }

    rc = pthread_join(child, NULL);
    if (rc != 0) {
        fprintf(stderr, "Thread join error: %s\n", strerror(rc));
        exit(1);
    }

    exit(0);
}
