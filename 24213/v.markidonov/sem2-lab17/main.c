#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "stack.h"

#define BUFFER_SIZE 80

stack st;

void* sorter(void* arg) {
    while (1) {
        stack_sort(&st);
        sleep(5);
    }
}

int main() {
    pthread_mutex_t mutex;
    int code = pthread_mutex_init(&mutex, NULL);
    if (code != 0) {
        fprintf(stderr, "mutex init: %s\n", strerror(code));
        exit(1);
    }

    stack_init(&st, &mutex);
    char buffer[BUFFER_SIZE + 1];
    pthread_t th;

    code = pthread_create(&th, NULL, sorter, NULL);
    if (code != 0) {
        fprintf(stderr, "creating thread: %s\n", strerror(code));
        exit(1);
    }

    int longline = 0;
    while (fgets(buffer, BUFFER_SIZE + 1, stdin)) {
        if (buffer[0] == '\n') {
            if (!longline) {
                stack_print(&st);
                printf("\n");
            }
            longline = 0;
            continue;
        }

        int len = strlen(buffer);

        if (len < BUFFER_SIZE) {
            longline = 0;
        } else if (buffer[len] != '\n') {
            longline = 1;
        }

        if (!longline) {
            buffer[--len] = '\0';
        }
        
        stack_push(&st, buffer, len + 1);
    }

    code = pthread_cancel(th);
    if (code != 0) {
        fprintf(stderr, "cancel thread: %s\n", strerror(code));
        exit(1);
    }

    code = pthread_join(th, NULL);
    if (code != 0) {
        fprintf(stderr, "join thread: %s\n", strerror(code));
        exit(1);
    }

    code = pthread_mutex_destroy(&mutex);
    if (code != 0) {
        fprintf(stderr, "mutex destroy: %s\n", strerror(code));
        exit(1);
    }

    return 0;
}
