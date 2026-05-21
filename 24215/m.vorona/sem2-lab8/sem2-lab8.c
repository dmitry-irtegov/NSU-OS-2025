#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifndef NUM_STEPS
#define NUM_STEPS 200000000
#endif

typedef struct {
    long thread_id;
    long num_threads;
} thread_data_t;

static void *compute_partial_sum(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;

    double *partial = malloc(sizeof(double));
    if (partial == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }

    *partial = 0.0;

    for (long i = data->thread_id; i < NUM_STEPS; i += data->num_threads) {
        *partial += 1.0 / (4.0 * i + 1.0);
        *partial -= 1.0 / (4.0 * i + 3.0);
    }

    pthread_exit(partial);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    errno = 0;
    long num_threads = strtol(argv[1], &endptr, 10);

    if (errno != 0 || *endptr != '\0' || num_threads <= 0) {
        fprintf(stderr, "Invalid thread count: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    thread_data_t *thread_data = malloc(sizeof(thread_data_t) * num_threads);

    if (threads == NULL || thread_data == NULL) {
        perror("malloc");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    for (long i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;

        int ret = pthread_create(&threads[i], NULL, compute_partial_sum, &thread_data[i]);
        if (ret != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
            for (long j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }
    }

    double pi = 0.0;

    for (long i = 0; i < num_threads; i++) {
        void *retval = NULL;

        int ret = pthread_join(threads[i], &retval);
        if (ret != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(ret));
            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }

        if (retval == NULL) {
            fprintf(stderr, "Thread %ld returned NULL\n", i);
            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }

        double *partial = (double *)retval;
        pi += *partial;
        free(partial);
    }

    pi *= 4.0;

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    double elapsed = (end.tv_sec - start.tv_sec)
                   + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("pi done - %.15g\n", pi);
    printf("time: %.6f sec\n", elapsed);

    free(threads);
    free(thread_data);

    return 0;
}