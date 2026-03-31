#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef NUM_ITERATIONS
#define NUM_ITERATIONS 100000000L
#endif

typedef struct {
    long start;
    long end;
} thread_arg_t;

static void* compute_partial(void *arg)
{
    thread_arg_t* targ = (thread_arg_t*)arg;
    double* result = malloc(sizeof(double));
    if (result == NULL) {
        pthread_exit(NULL);
    }

    double sum = 0.0;
    long i;
    for (i = targ->start; i < targ->end; i++) {
        double term = 1.0 / (2.0 * i + 1.0);
        if (i % 2 == 0)
            sum += term;
        else
            sum -= term;
    }

    *result = sum;
    pthread_exit(result);
    return NULL;
}

int main(int argc, char *argv[])
{
    int num_threads, i;
    pthread_t* tids;
    thread_arg_t* args;
    double pi = 0.0;

    if (argc != 2) {
        fprintf(stderr, "Using: %s <threads count>\n", argv[0]);
        return 1;
    }

    num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Threads count should be more than 0\n");
        return 1;
    }

    tids = malloc(num_threads * sizeof(pthread_t));
    args = malloc(num_threads * sizeof(thread_arg_t));
    if (tids == NULL || args == NULL) {
        perror("malloc");
        return 1;
    }

    long chunk = NUM_ITERATIONS / num_threads;
    for (i = 0; i < num_threads; i++) {
        args[i].start = i * chunk;
        args[i].end = (i == num_threads - 1) ? NUM_ITERATIONS : (i + 1) * chunk;
        if (pthread_create(&tids[i], NULL, compute_partial, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (i = 0; i < num_threads; i++) {
        void* retval;
        if (pthread_join(tids[i], &retval) != 0) {
            perror("pthread_join");
            return 1;
        }
        if (retval != NULL) {
            double *partial = (double *)retval;
            pi += *partial;
            free(partial);
        }
    }

    pi *= 4.0;
    printf("Iterations : %ld\n", (long)NUM_ITERATIONS);
    printf("Threads  : %d\n", num_threads);
    printf("pi       : %.15f\n", pi);

    free(tids);
    free(args);
    return 0;
}