#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_THREADS 4
#define STR_COUNT 3

typedef struct {
    const char **strs;
    int count;
} ThreadData;

void *printer(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = 0; i < data->count; i++) {
        fprintf(stderr, "%s\n", data->strs[i]);
    }
    pthread_exit(NULL);
}

int main() {
    const char *data[NUM_THREADS][STR_COUNT] = {
        {"1 Thread, 1 string", "1 Thread, 2 string", "1 Thread, 3 string"},
        {"2 Thread, 1 string", "2 Thread, 2 string", "2 Thread, 3 string"},
        {"3 Thread, 1 string", "3 Thread, 2 string", "3 Thread, 3 string"},
        {"4 Thread, 1 string", "4 Thread, 2 string", "4 Thread, 3 string"},
    };

    pthread_t threads[NUM_THREADS];
    ThreadData thread_args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].strs = data[i];
        thread_args[i].count = STR_COUNT;

        int err = pthread_create(&threads[i], NULL, printer, &thread_args[i]);
        if (err != 0) {
            fprintf(stderr, "Error creating thread %d: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}