#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

void* thread_printer(void *arg) {
    while (1) {
        fprintf(stderr, "hard work...\n");
        usleep(10000);
    }

    return NULL;
}

int main() {
    pthread_t worker_id;

    int status = pthread_create(&worker_id, NULL, thread_printer, NULL);

    if (status != 0) {
        fprintf(stderr, "Error: pthread_create: %s", strerror(status));
        return 1;
    }

    sleep(2);

    status = pthread_cancel(worker_id);

    if (status != 0) {
        fprintf(stderr, "Error: pthread_cancel: %s", strerror(status));
        return 1;
    }
    
    status = pthread_join(worker_id, NULL);
    
    if (status != 0) {
        fprintf(stderr, "Error: pthread_join: %s", strerror(status));
        return 1;
    }

    return 0;
}