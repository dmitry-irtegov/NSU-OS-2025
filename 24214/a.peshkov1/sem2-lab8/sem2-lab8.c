#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TOTAL_STEPS 200000000

typedef struct {
    long from;
    long to;
} task_range_t;

void* leibniz_worker(void* arg) {
    task_range_t* range = (task_range_t*)arg;
    double* partial_sum = malloc(sizeof(double));
    
    if (!partial_sum) pthread_exit(NULL);

    *partial_sum = 0.0;
    for (long i = range->from; i < range->to; i++) {
        *partial_sum += (1.0 / (4.0 * i + 1.0)) - (1.0 / (4.0 * i + 3.0));
    }

    pthread_exit(partial_sum);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n_threads = atoi(argv[1]);
    if (n_threads < 1) n_threads = 1;

    pthread_t* pool = malloc(sizeof(pthread_t) * n_threads);
    task_range_t* args = malloc(sizeof(task_range_t) * n_threads);
    
    long step_size = TOTAL_STEPS / n_threads;

    for (int i = 0; i < n_threads; i++) {
        args[i].from = i * step_size;
        if (i == n_threads - 1) {
	    args[i].to = TOTAL_STEPS;
	} else {
            args[i].to = (i + 1) * step_size;
	}

        if (pthread_create(&pool[i], NULL, leibniz_worker, &args[i]) != 0) {
            perror("Thread creation failed");
            return EXIT_FAILURE;
        }
    }

    double pi_total = 0.0;
    for (int i = 0; i < n_threads; i++) {
        double* res;
        pthread_join(pool[i], (void**)&res);
        if (res) {
            pi_total += *res;
            free(res);
        }
    }

    printf("Result: %.15f\n", pi_total * 4.0);

    free(pool);
    free(args);
    return EXIT_SUCCESS;
}