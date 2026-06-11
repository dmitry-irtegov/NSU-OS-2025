#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#define CHECK_INTERVAL 1000000L

static volatile sig_atomic_t stop_requested = 0;
static pthread_barrier_t barrier;
static int *stop_flags = NULL;

typedef struct {
    long thread_id;
    long num_threads;
} thread_data_t;

static void sigint_handler(int signo) {
    (void)signo;
    stop_requested = 1;
}

static void *compute_partial_sum(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    long id = data->thread_id;
    long n_threads = data->num_threads;

    double *partial = malloc(sizeof(double));
    if (partial == NULL) {
        pthread_exit(NULL);
    }

    *partial = 0.0;

    long i = id;
    long block = 0;

    while (1) {
        for (long step = 0; step < CHECK_INTERVAL; step++, i += n_threads) {
            *partial += 1.0 / (4.0 * i + 1.0);
            *partial -= 1.0 / (4.0 * i + 3.0);
        }

        int slot = (int)(block % 2);

        stop_flags[id * 2 + slot] = stop_requested ? 1 : 0;

        int rc = pthread_barrier_wait(&barrier);
        if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
            free(partial);
            pthread_exit(NULL);
        }

        int need_finish = 0;

        for (long j = 0; j < n_threads; j++) {
            if (stop_flags[j * 2 + slot]) {
                need_finish = 1;
                break;
            }
        }

        if (need_finish) {
            pthread_exit(partial);
        }

        block++;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }

    char *endptr = NULL;
    errno = 0;
    long num_threads = strtol(argv[1], &endptr, 10);

    if (errno != 0 || *endptr != '\0' || num_threads <= 0) {
        fprintf(stderr, "Invalid thread count: %s\n", argv[1]);
        return 1;
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    thread_data_t *thread_data = malloc(sizeof(thread_data_t) * num_threads);
    stop_flags = calloc((size_t)num_threads * 2, sizeof(int));

    if (threads == NULL || thread_data == NULL || stop_flags == NULL) {
        perror("malloc");
        free(threads);
        free(thread_data);
        free(stop_flags);
        return 1;
    }

    int ret = pthread_barrier_init(&barrier, NULL, (unsigned)num_threads);
    if (ret != 0) {
        fprintf(stderr, "pthread_barrier_init failed: %s\n", strerror(ret));
        free(threads);
        free(thread_data);
        free(stop_flags);
        return 1;
    }

    for (long i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;

        ret = pthread_create(&threads[i], NULL, compute_partial_sum, &thread_data[i]);
        if (ret != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
            exit(1);
        }
    }

    printf("Running... Press Ctrl+C to stop\n");
    fflush(stdout);

    double pi = 0.0;

    for (long i = 0; i < num_threads; i++) {
        void *retval = NULL;

        ret = pthread_join(threads[i], &retval);
        if (ret != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(ret));
            pthread_barrier_destroy(&barrier);
            free(threads);
            free(thread_data);
            free(stop_flags);
            return 1;
        }

        if (retval == NULL) {
            fprintf(stderr, "Thread %ld returned NULL\n", i);
            pthread_barrier_destroy(&barrier);
            free(threads);
            free(thread_data);
            free(stop_flags);
            return 1;
        }

        double *partial = (double *)retval;
        pi += *partial;
        free(partial);
    }

    pi *= 4.0;

    printf("pi done - %.15g\n", pi);

    pthread_barrier_destroy(&barrier);
    free(threads);
    free(thread_data);
    free(stop_flags);

    return 0;
}