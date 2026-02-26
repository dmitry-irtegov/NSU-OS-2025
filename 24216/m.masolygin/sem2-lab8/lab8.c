#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define num_steps 200000000

typedef struct {
    long start;
    long end;
} thread_args_t;

void* calc_pi_part(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;

    double* partial_sum = malloc(sizeof(double));
    if (partial_sum == NULL) {
        fprintf(stderr, "Error allocating memory for partial sum\n");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;
    for (long i = args->start; i < args->end; i++) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
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

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_args_t* args = malloc(num_threads * sizeof(thread_args_t));

    if (threads == NULL || args == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        free(threads);
        free(args);
        return 1;
    }

    long steps_per_thread = num_steps / num_threads;
    long remainder = num_steps % num_threads;

    long start = 0;
    for (int i = 0; i < num_threads; i++) {
        args[i].start = start;
        args[i].end = start + steps_per_thread + (i < remainder ? 1 : 0);
        start = args[i].end;

        int error = pthread_create(&threads[i], NULL, calc_pi_part, &args[i]);
        if (error != 0) {
            fprintf(stderr, "Error creating thread %d: %s\n", i, strerror(error));
            free(threads);
            free(args);
            return 1;
        }
    }

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
