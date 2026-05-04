#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_STEPS 200000000

typedef struct {
    int start;
    int end;
} ThreadData;

void* worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    double* sum = malloc(sizeof(double));
    *sum = 0.0;

    for (int i = data->start; i < data->end; i++) {
        *sum += 1.0 / (i * 4.0 + 1.0);
        *sum -= 1.0 / (i * 4.0 + 3.0);
    }

    pthread_exit(sum);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <num_threads>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData* data = malloc(num_threads * sizeof(ThreadData));

    int chunk = NUM_STEPS / num_threads;

    for (int i = 0; i < num_threads; i++) {
        data[i].start = i * chunk;
        data[i].end = (i == num_threads - 1) ? NUM_STEPS : (i + 1) * chunk;

        pthread_create(&threads[i], NULL, worker, &data[i]);
    }

    double pi = 0.0;

    for (int i = 0; i < num_threads; i++) {
        double* partial;
        pthread_join(threads[i], (void**)&partial);

        pi += *partial;
        free(partial);
    }

    pi *= 4.0;

    printf("pi done - %.15g\n", pi);

    free(threads);
    free(data);

    return 0;
}

