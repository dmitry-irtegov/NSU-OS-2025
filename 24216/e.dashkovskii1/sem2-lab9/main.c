#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#define CHECK_INTERVAL 1000000
#define EXTRA_ITERS    50000000

static sig_atomic_t stop_flag = 0;

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

void sigint_handler(int sig) {
    (void)sig;
    stop_flag = 1;
}

void *calc_partial(void *arg) {
    thread_arg *targ = (thread_arg *)arg;
    double *partial_sum = (double *)malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;
    long i = targ->idx;
    int step = targ->total_threads;

    while (!stop_flag) {
        for (int k = 0; k < CHECK_INTERVAL; k++) {
            *partial_sum += 1.0 / (i * 4.0 + 1.0);
            *partial_sum -= 1.0 / (i * 4.0 + 3.0);
            i += step;
        }
    }

    for (long k = 0; k < EXTRA_ITERS; k++) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
        i += step;
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

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    thread_arg *args = (thread_arg *)malloc(num_threads * sizeof(thread_arg));
    if (threads == NULL || args == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    printf("Calculating pi with %d threads, press Ctrl+C to stop...\n", num_threads);

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
