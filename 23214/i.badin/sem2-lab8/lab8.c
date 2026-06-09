#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NUM_STEPS
#define NUM_STEPS 200000000L
#endif

typedef struct {
    long start;
    long end;
} ThreadData;

void* calculate_partial_sum(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    double* partial_sum = malloc(sizeof(double));

    if (partial_sum == NULL) {
        fprintf(stderr, "Failed to allocate memory for partial sum\n");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;
    for (long i = data->start; i < data->end; i++) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }

    pthread_exit(partial_sum);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <threads_count>\n", argv[0]);
        return EXIT_FAILURE;
    }

    errno = 0;
    char* endptr = NULL;
    long threads_count_long = strtol(argv[1], &endptr, 10);

    if (errno == ERANGE || *endptr != '\0' || threads_count_long <= 0 || threads_count_long > INT_MAX) {
        fprintf(stderr, "Threads count must be a positive integer\n");
        return EXIT_FAILURE;
    }

    int threads_count = (int)threads_count_long;

    pthread_t* threads = malloc(threads_count * sizeof(pthread_t));
    ThreadData* thread_data = malloc(threads_count * sizeof(ThreadData));

    if (threads == NULL || thread_data == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    long steps_per_thread = NUM_STEPS / threads_count;
    long rest_steps = NUM_STEPS % threads_count;

    int created_threads = 0;

    for (int i = 0; i < threads_count; i++) {
        long additional_step = i < rest_steps ? 1L : 0L;

        thread_data[i].start = i * steps_per_thread + (i < rest_steps ? i : rest_steps);
        thread_data[i].end = thread_data[i].start + steps_per_thread + additional_step;

        int status = pthread_create(&threads[i], NULL, calculate_partial_sum, &thread_data[i]);

        if (status != 0) {
            fprintf(stderr, "Failed to create thread %d: %s\n", i, strerror(status));
            break;
        }

        created_threads++;
    }

    double pi = 0.0;
    int failed = 0;

    for (int i = 0; i < created_threads; i++) {
        double* partial_sum = NULL;

        int status = pthread_join(threads[i], (void**)&partial_sum);

        if (status != 0) {
            fprintf(stderr, "Failed to join thread %d: %s\n", i, strerror(status));
            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }

        if (partial_sum == NULL) {
            failed = 1;
            continue;
        }

        pi += *partial_sum;
        free(partial_sum);
    }

    free(threads);
    free(thread_data);

    if (created_threads != threads_count || failed) {
        return EXIT_FAILURE;
    }

    pi *= 4.0;
    printf("pi done - %.15g\n", pi);

    return EXIT_SUCCESS;
}
