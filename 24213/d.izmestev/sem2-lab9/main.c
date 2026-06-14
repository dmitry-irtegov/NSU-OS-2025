#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#define INTERVAL 1000000

typedef struct {
    long long start;
    double partial_sum;
} ThreadData;

pthread_barrier_t epoch_barrier;
volatile sig_atomic_t stop_flag = 0; 
int agreed_stop = 0;
int num_threads = 0;

void handle_sigint(int sig) {
    stop_flag = 1;
}

void* calc_pi_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    double local_sum = 0.0;
    long long iterations = 0;

    for (long long i = data->start; ; i += num_threads) {
        local_sum += 1.0 / (i * 4.0 + 1.0);
        local_sum -= 1.0 / (i * 4.0 + 3.0);

        iterations++;

        if (iterations % INTERVAL == 0) {
            int res = pthread_barrier_wait(&epoch_barrier);
            if (res == PTHREAD_BARRIER_SERIAL_THREAD) {
                agreed_stop = stop_flag;
            } else if (res != 0) {
                fprintf(stderr, "barrier wait: %s\n", strerror(res));
                exit(1);
            }

            res = pthread_barrier_wait(&epoch_barrier);
            if (res != 0 && res != PTHREAD_BARRIER_SERIAL_THREAD) {
                fprintf(stderr, "barrier wait: %s\n", strerror(res));
                exit(1);
            }

            if (agreed_stop) {
                break;
            }
        }
    } 

    data->partial_sum = local_sum;
    return data;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be integer and > 0\n");
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigint;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    int err = pthread_barrier_init(&epoch_barrier, NULL, (unsigned)num_threads);
    if (err != 0) {
        fprintf(stderr, "barrier init: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData* tdata = malloc(num_threads * sizeof(ThreadData));
    if (threads == NULL || tdata == NULL) {
        perror("malloc");
        free(threads); free(tdata);
        pthread_barrier_destroy(&epoch_barrier);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_threads; i++) {
        tdata[i].start = i;
        tdata[i].partial_sum = 0.0;

        int err = pthread_create(&threads[i], NULL, calc_pi_worker, &tdata[i]);
        if (err != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            free(threads); free(tdata);
            pthread_barrier_destroy(&epoch_barrier);
            return EXIT_FAILURE;
        }
    }

    double final_pi = 0.0;

    for (int i = 0; i < num_threads; i++) {
        ThreadData* res = NULL;
        err = pthread_join(threads[i], (void**)&res);
        if (err != 0) {
            fprintf(stderr, "pthread join: %s\n", strerror(err));
            free(threads); free(tdata);
            pthread_barrier_destroy(&epoch_barrier);
            return EXIT_FAILURE;
        }
        final_pi += res->partial_sum;
    }
    final_pi *= 4.0;

    printf("\nPi:    %.15g\n", final_pi);

    free(threads);
    free(tdata);
    err = pthread_barrier_destroy(&epoch_barrier);
    if (err != 0) {
        fprintf(stderr, "barrier destroy: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
