#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#define CHECK_INTERVAL 1000000L

static volatile sig_atomic_t stop_requested = 0;

typedef struct {
    long thread_id;
    long num_threads;
} thread_data_t;

static void sigint_handler(int signo)
{
    (void)signo;
    stop_requested = 1;
}

void *calc_pi(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    long id = data->thread_id;
    long n_threads = data->num_threads;

    double *partial_sum = malloc(sizeof(double));
    if (partial_sum == NULL) {
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;

    long i = id;
    while (1) {
        long limit = i + CHECK_INTERVAL * n_threads;
        for (; i < limit; i += n_threads) {
            *partial_sum += 1.0 / (i * 4.0 + 1.0);
            *partial_sum -= 1.0 / (i * 4.0 + 3.0);
        }

        if (stop_requested) {
            break;
        }
    }

    pthread_exit(partial_sum);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Use: %s <number of threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threades need to be positive.\n");
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    thread_data_t *thread_data = malloc((size_t)num_threads * sizeof(thread_data_t));

    if (threads == NULL || thread_data == NULL) {
        perror("malloc");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_threads; ++i) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;

        if (pthread_create(&threads[i], NULL, calc_pi, &thread_data[i]) != 0) {
            perror("pthread_create");
            stop_requested = 1;

            for (int j = 0; j < i; ++j) {
                void *status = NULL;
                pthread_join(threads[j], &status);
                free(status);
            }

            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }
    }

    double pi_sum = 0.0;

    for (int i = 0; i < num_threads; ++i) {
        void *status = NULL;
        if (pthread_join(threads[i], &status) != 0) {
            perror("pthread_join");
            continue;
        }

        if (status != NULL) {
            double *sum_from_thread = (double *)status;
            pi_sum += *sum_from_thread;
            free(sum_from_thread);
        }
    }

    printf("\nPI approximation: %.15g\n", pi_sum * 4.0);

    free(threads);
    free(thread_data);

    return EXIT_SUCCESS;
}