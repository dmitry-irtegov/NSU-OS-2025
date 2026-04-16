#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define NUM_STEPS 200000000

typedef struct {
    int idx;
    int total_threads;
} thread_arg;

void handle_error(int en, const char *msg) {
    if (en != 0) {
        fprintf(stderr, "%s: %s\n", msg, strerror(en));
        exit(EXIT_FAILURE);
    }   
}

void *calc_partial(void *arg) {
    thread_arg *targ = (thread_arg *)arg;
    double *partial_sum = (double *)malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    *partial_sum = 0.0;
    for (long i = targ->idx; i < NUM_STEPS; i += targ->total_threads) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }

    pthread_exit(partial_sum);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads < 1) {
        fprintf(stderr, "Number of threads must be >= 1\n");
        return EXIT_FAILURE;
    }

    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    thread_arg *args = (thread_arg *)malloc(num_threads * sizeof(thread_arg));
    if (threads == NULL || args == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_threads; i++) {
        args[i].idx = i;
        args[i].total_threads = num_threads;
        handle_error(pthread_create(&threads[i], NULL, calc_partial, &args[i]),
                     "pthread_create");
    }

    double pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        double *partial;
        handle_error(pthread_join(threads[i], (void **)&partial), "pthread_join");
        if (partial != NULL) {
            pi += *partial;
            free(partial);
        }
    }

    pi *= 4.0;
    printf("pi = %.15g\n", pi);

    free(threads);
    free(args);
    return EXIT_SUCCESS;
}
