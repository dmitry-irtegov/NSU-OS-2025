#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define NUM_STEPS 200000000

typedef struct {
    int start;
    int end;
    int step;
    double sum;
} worker_data_t;

void *worker_run(void *arg) {
    worker_data_t *data = (worker_data_t *)arg;

    double pi = 0;
    for (int i = data->start; i < data->end; i += data->step) {
        pi += 1.0 / (i * 4.0 + 1.0);
        pi -= 1.0 / (i * 4.0 + 3.0);
    }

    data->sum = pi;
    return data;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s NUM_THREADS\n", argv[0]);
        return 1;
    }
    int num_workers = -1;
    sscanf(argv[1], "%d", &num_workers);
    if (num_workers < 1 || num_workers > 100) {
        fprintf(stderr, "NUM_THREADS must be an integer between 1 and 100.\n");
        return 1;
    }

    static worker_data_t worker_data[100];
    static pthread_t workers[100];

    for (int i = 0; i < num_workers; i++) {
        worker_data[i] = (worker_data_t){
            .start = i,
            .end = NUM_STEPS,
            .step = num_workers,
        };
        int error =
            pthread_create(&workers[i], NULL, worker_run, &worker_data[i]);
        if (error) {
            fprintf(stderr, "could not create thread: %s\n", strerror(error));
            return 1;
        }
    }

    double result = 0;
    for (int i = 0; i < num_workers; i++) {
        worker_data_t *data;
        pthread_join(workers[i], (void **)&data);
        result += data->sum;
    }
    fprintf(stdout, "pi done - %.15g\n", result * 4);
    return 0;
}
