#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define num_steps 200000000

typedef struct {
    int thread_id;
    int thread_count;
} thread_arg_t;


void *thread_func(void *arg) {
    thread_arg_t *data = (thread_arg_t *)arg;

    int tid = data->thread_id;
    int nthreads = data->thread_count;

    double *partial_sum = malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;

    for (int i = tid; i < num_steps; i += nthreads) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }

    pthread_exit(partial_sum);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Error: not enough arguments\n");
        return 1;
    }

    int thread_count = atoi(argv[1]);
    if (thread_count <= 0) {
        fprintf(stderr, "Error: thread count must be positive\n");
        return 1;
    }

    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    thread_arg_t *args = malloc(thread_count * sizeof(thread_arg_t));

    if (threads == NULL || args == NULL) {
        perror("malloc");
        free(threads);
        free(args);
        return 1;
    }

    for (int i = 0; i < thread_count; i++) {
        args[i].thread_id = i;
        args[i].thread_count = thread_count;

        int res = pthread_create(&threads[i], NULL, thread_func, &args[i]);
        if (res != 0) {
            fprintf(stderr, "pthread_create failed for thread %d\n", i);

            for (int j = 0; j < i; j++) {
                void *result;
                pthread_join(threads[j], &result);
                free(result);
            }

            free(threads);
            free(args);
            return 1;
        }
    }

    double pi = 0.0;

    for (int i = 0; i < thread_count; i++) {
        void *result;
        int res = pthread_join(threads[i], &result);
        if (res != 0) {
            fprintf(stderr, "error: pthread_join failed for thread %d\n", i);
            continue;
        }

        if (result != NULL) {
            pi += *(double *)result;
            free(result);
        }
    }

    pi *= 4.0;

    printf("pi done - %.15g\n", pi);

    free(threads);
    free(args);

    return 0;
}