#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>

#define CHECK_INTERVAL 1000000

volatile sig_atomic_t stop = 0;

typedef struct
{
    int thread_id;
    int num_threads;
    double partial_sum;
    long long iterations_done;
} data_t;

void sigint_handler(int sig)
{
    if (sig == SIGINT)
    {
        stop = 1;
    }
}

void *compute_pi_part(void *arg)
{
    data_t *data = (data_t *)arg;
    double sum = 0.0;
    long long check_counter = 0;
    long long i = data->thread_id;

    while (1)
    {
        sum += 1.0 / (i * 4.0 + 1.0);
        sum -= 1.0 / (i * 4.0 + 3.0);

        data->iterations_done++;
        check_counter++;
        i += data->num_threads;

        if (check_counter >= CHECK_INTERVAL)
        {
            check_counter = 0;
            if (stop)
            {
                break;
            }
        }
    }

    data->partial_sum = sum;

    void *result = malloc(sizeof(double));
    if (result != NULL)
    {
        *((double *)result) = data->partial_sum;
    }

    pthread_exit(result);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0 || num_threads > 100)
    {
        fprintf(stderr, "Number of threads must be > 0 and < 100\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction failed");
        return 1;
    }

    pthread_t threads[num_threads];
    data_t thread_data[num_threads];

    for (int i = 0; i < num_threads; i++)
    {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].partial_sum = 0.0;
        thread_data[i].iterations_done = 0;

        if (pthread_create(&threads[i], NULL, compute_pi_part, &thread_data[i]) != 0)
        {
            perror("pthread_create failed");
            return 1;
        }
    }

    double pi = 0.0;
    long long total_iterations = 0;

    for (int i = 0; i < num_threads; i++)
    {
        void *partial_sum_ptr;
        if (pthread_join(threads[i], &partial_sum_ptr) != 0)
        {
            perror("pthread_join failed");
            return 1;
        }

        if (partial_sum_ptr != NULL)
        {
            pi += *((double *)partial_sum_ptr);
            free(partial_sum_ptr);
        }

        total_iterations += thread_data[i].iterations_done;
    }

    pi = pi * 4.0;

    printf("pi: %.15g \n", pi);
    printf("iterations: %lld\n", total_iterations);
    return 0;
}