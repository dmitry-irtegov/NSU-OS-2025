#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_INTERVAL 1000000LL

static volatile sig_atomic_t stop_flag = 0;

typedef struct {
    int thread_id;
    int num_threads;
} thread_args_t;

static void sigint_handler(int sig) {
    (void)sig;
    stop_flag = 1;
}

void* calc_pi_part(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;

    double* partial_sum = malloc(sizeof(double));
    if (partial_sum == NULL) {
        fprintf(stderr, "Error allocating memory for partial sum\n");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;

    long long block = args->thread_id;
    while (!stop_flag) {
        long long start = block * CHECK_INTERVAL;
        long long end = start + CHECK_INTERVAL;
        for (long long i = start; i < end; i++) {
            *partial_sum += 1.0 / (i * 4.0 + 1.0);
            *partial_sum -= 1.0 / (i * 4.0 + 3.0);
        }
        block += args->num_threads;
    }

    pthread_exit(partial_sum);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("sigaction");
        return 1;
    }

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_args_t* args = malloc(num_threads * sizeof(thread_args_t));

    if (threads == NULL || args == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        free(threads);
        free(args);
        return 1;
    }

    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].num_threads = num_threads;

        int error = pthread_create(&threads[i], NULL, calc_pi_part, &args[i]);
        if (error != 0) {
            fprintf(stderr, "Error creating thread %d: %s\n", i, strerror(error));
            free(threads);
            free(args);
            return 1;
        }
    }

    printf("Running... press Ctrl+C to stop and print pi approximation.\n");

    double pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        double* partial_sum = NULL;
        int error = pthread_join(threads[i], (void**)&partial_sum);
        if (error != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i, strerror(error));
            free(threads);
            free(args);
            return 1;
        }
        if (partial_sum != NULL) {
            pi += *partial_sum;
            free(partial_sum);
        }
    }

    pi *= 4.0;
    printf("pi done - %.15g\n", pi);

    free(threads);
    free(args);
    return 0;
}
