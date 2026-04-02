#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define NUM_STEPS 200000000

int nthreads;

void *calculate(void *arg) {
    int index = *(int *)arg;
    double partial = 0.0;

    for (long long i = index; i < NUM_STEPS; i += nthreads) {
        partial += 1.0 / (i * 4.0 + 1.0);
        partial -= 1.0 / (i * 4.0 + 3.0);
    }

    double *result = (double *)malloc(sizeof(double));
    if (result == NULL) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        return NULL;
    }
    *result = partial;

    pthread_exit(result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    nthreads = atoi(argv[1]);
    if (nthreads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return EXIT_FAILURE;
    }

    pthread_t *threads = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
    int *indices = (int *)malloc(nthreads * sizeof(int));

    for (int i = 0; i < nthreads; i++) {
        indices[i] = i;
        if (pthread_create(&threads[i], NULL, calculate, &indices[i]) != 0) {
            fprintf(stderr, "Error creating thread %d: %s\n", i, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    double pi = 0.0;
    for (int i = 0; i < nthreads; i++) {
        void *retval;
        if (pthread_join(threads[i], &retval) != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i, strerror(errno));
            return EXIT_FAILURE;
        }
        if (retval != NULL) {
            pi += *(double *)retval;
            free(retval);
        }
    }

    pi *= 4.0;
    printf("pi done - %.15g\n", pi);

    free(threads);
    free(indices);

    return EXIT_SUCCESS;
}
