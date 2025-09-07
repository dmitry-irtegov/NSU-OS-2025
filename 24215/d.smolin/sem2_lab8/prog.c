#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define NUM_STEPS 20000000

typedef struct {
    int  id;
    long start;
    long end;
    double *sum;
} thread_data_t;

void *leibniz_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    double partial = 0.0;

    for (long i = data->start; i < data->end; i++) {
        partial += 1.0 / (i * 4.0 + 1.0);
        partial -= 1.0 / (i * 4.0 + 3.0);
    }

    *data->sum = partial;
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s \n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        return EXIT_FAILURE;
    }

    pthread_t *threads   = malloc(num_threads * sizeof(pthread_t));
    double *partials   = malloc(num_threads * sizeof(double));
    thread_data_t *data  = malloc(num_threads * sizeof(thread_data_t));

    long step_block = NUM_STEPS / num_threads;

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    for (int t = 0; t < num_threads; t++) {
        long start = t * step_block;
        long end   = (t == num_threads - 1) ? NUM_STEPS : (t + 1) * step_block;

        data[t].id = t;
        data[t].start = start;
        data[t].end = end;
        data[t].sum = &partials[t];

        if (pthread_create(&threads[t], NULL, leibniz_thread, &data[t]) != 0) {
            free(threads);
            free(partials);
            free(data);
            return EXIT_FAILURE;
        }
    }

    double total = 0.0;
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
        total += partials[t];
    }

    gettimeofday(&t1, NULL);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1e6;

    double pi = total * 4.0;
    printf("pi done - %.15g\n", pi);
    printf("time: %.6f sec\n", elapsed);

    free(threads);
    free(partials);
    free(data);

    return EXIT_SUCCESS;
}
