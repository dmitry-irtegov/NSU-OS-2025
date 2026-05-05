#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_STEPS 200000000

typedef struct {
    long start;
    long end;
} thread_data_t;

void* worker(void* arg) {
    thread_data_t* data = (thread_data_t*) arg;

    double* partial_sum = (double*) malloc(sizeof(double));
    if (!partial_sum) {
        perror("malloc");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;

    for (long i = data->start; i < data->end; i++) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }

    pthread_exit(partial_sum);
}

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Use: %s <num_threads>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        printf("Invalid number of threads\n");
        return 1;
    }

    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    thread_data_t* tdata = malloc(sizeof(thread_data_t) * num_threads);

    long chunk = NUM_STEPS / num_threads;

    for (int i = 0; i < num_threads; i++) {
        tdata[i].start = i * chunk;
        tdata[i].end = (i == num_threads - 1)
                       ? NUM_STEPS
                       : (i + 1) * chunk;

        pthread_create(&threads[i], NULL, worker, &tdata[i]);
    }

    double pi = 0.0;

    for (int i = 0; i < num_threads; i++) {
        double* partial;
        pthread_join(threads[i], (void**)&partial);

        pi += *partial;
        free(partial);
    }

    pi *= 4.0;

    printf("pi = %.15g\n", pi);

    free(threads);
    free(tdata);

    return 0;
}