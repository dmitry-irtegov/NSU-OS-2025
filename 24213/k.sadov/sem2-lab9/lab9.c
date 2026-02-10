#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define CHECK 1048576

typedef struct {
    long long start_index;
    double partial_sum;
} Data;

pthread_barrier_t end_barrier;
atomic_int stop_flag = ATOMIC_VAR_INIT(0);
int nthreads;
int agreed_stop;

void sigint_handler(int sig) {
    atomic_store_explicit(&stop_flag, 1, memory_order_relaxed);
}

void *calculate(void *param) {
    Data *data = (Data *)param;
    double local_sum = 0.0;
    long long i = data->start_index;
    long long iterations = 0;

    for (;; i += nthreads) {
        local_sum += 1.0 / (i * 4.0 + 1.0);
        local_sum -= 1.0 / (i * 4.0 + 3.0);

        iterations++;
        if ((iterations & (CHECK - 1)) == 0) {
            int rc = pthread_barrier_wait(&end_barrier);
            if (rc == PTHREAD_BARRIER_SERIAL_THREAD) {
                agreed_stop = atomic_load_explicit(&stop_flag, memory_order_relaxed);
            }
            pthread_barrier_wait(&end_barrier);
            if (agreed_stop) {
                break;
            }
        }
    }
    data->partial_sum = local_sum;
    printf("%lld\n", iterations);
    return param;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[1]);
    if (nthreads < 1) {
        fprintf(stderr, "Number of threads must be >= 1\n");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    int err;
    pthread_barrier_init(&end_barrier, NULL, (unsigned)nthreads);
    pthread_t *ids = malloc(nthreads * sizeof(pthread_t));
    Data *params = malloc(nthreads * sizeof(Data));

    for (int i = 0; i < nthreads; i++) {
        params[i].start_index = i;
        params[i].partial_sum = 0.0;

        err = pthread_create(&ids[i], NULL, calculate, &params[i]);
        if (err != 0) {
            fprintf(stderr, "Error while creating %d thread: %s\n", i, strerror(err));
            free(ids);
            free(params);
            pthread_barrier_destroy(&end_barrier);
            exit(EXIT_FAILURE);
        }
    }

    double pi = 0.0;

    for (int i = 0; i < nthreads; i++) {
        Data *res;
        err = pthread_join(ids[i], (void **)&res);
        if (err != 0) {
            fprintf(stderr, "Error while joining %d thread: %s\n", i, strerror(err));
        } else {
            pi += res->partial_sum;
        }
    }

    free(params);
    free(ids);

    pi *= 4.0;
    printf("\npi = %.15f\n", pi);

    pthread_barrier_destroy(&end_barrier);
    return EXIT_SUCCESS;
}
