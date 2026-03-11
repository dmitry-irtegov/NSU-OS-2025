#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#define CHUNK_STEPS 200000000L
#define CHECK_INTERVAL 1000000L

static volatile sig_atomic_t stop_requested = 0;

typedef struct {
    long start_step;
    long max_steps;
} thread_data_t;

static void sigint_handler(int signo)
{
    (void)signo;
    stop_requested = 1;
}

void *calc_pi(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    long start = data->start_step;
    long max_steps = data->max_steps;

    double *partial_sum = (double *)malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("malloc error in thread");
        pthread_exit(NULL);
    }
    *partial_sum = 0.0;

    long i;
    for (i = 0; i < max_steps; ++i) {
        long k = start + i;
        double term1 = 1.0 / (k * 4.0 + 1.0);
        double term2 = 1.0 / (k * 4.0 + 3.0);
        *partial_sum += term1 - term2;

        if ((i % CHECK_INTERVAL) == 0) {
            if (stop_requested) {
                long extra = CHECK_INTERVAL;
                long j;
                for (j = 0; j < extra && (i + j) < max_steps; ++j) {
                    long kk = start + i + j;
                    double t1 = 1.0 / (kk * 4.0 + 1.0);
                    double t2 = 1.0 / (kk * 4.0 + 3.0);
                    *partial_sum += t1 - t2;
                }
                i += extra;
                break;
            }
        }
    }

    pthread_exit((void *)partial_sum);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <количество потоков>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Количество потоков должно быть положительным числом.\n");
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    thread_data_t *thread_data = (thread_data_t *)malloc(num_threads * sizeof(thread_data_t));
    if (threads == NULL || thread_data == NULL) {
        perror("malloc error");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    double pi_sum = 0.0;
    long current_step = 0;

    while (!stop_requested) {
        for (int i = 0; i < num_threads; ++i) {
            thread_data[i].start_step = current_step + i * (CHUNK_STEPS / num_threads);
            thread_data[i].max_steps  = CHUNK_STEPS / num_threads;
        }

        long base = CHUNK_STEPS / num_threads;
        long rest = CHUNK_STEPS - base * num_threads;
        thread_data[num_threads - 1].max_steps += rest;

        current_step += CHUNK_STEPS;

        for (int i = 0; i < num_threads; ++i) {
            if (pthread_create(&threads[i], NULL, calc_pi, &thread_data[i]) != 0) {
                perror("pthread_create error");
                stop_requested = 1;
                num_threads = i;
                break;
            }
        }

        for (int i = 0; i < num_threads; ++i) {
            void *status = NULL;
            if (pthread_join(threads[i], &status) != 0) {
                perror("pthread_join error");
                continue;
            }
            if (status != NULL) {
                double *sum_from_thread = (double *)status;
                pi_sum += *sum_from_thread;
                free(sum_from_thread);
            }
        }

        if (stop_requested) {
            break;
        }
    }

    double pi = pi_sum * 4.0;
    printf("\nPI approximation: %.15g\n", pi);

    free(threads);
    free(thread_data);

    return EXIT_SUCCESS;
}
