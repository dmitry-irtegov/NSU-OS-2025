#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#ifndef TOTAL_ITERATIONS
#define TOTAL_ITERATIONS 200000000ULL
#endif

typedef struct {
    unsigned long long start_idx;
    unsigned long long end_idx;
} ThreadParams;

void* calculator(void* arg) {
    ThreadParams* params = (ThreadParams*)arg;
    
    double* chunk_sum = malloc(sizeof(double));
    if (!chunk_sum) {
        perror("Error malloc in calculator");
        pthread_exit(NULL);
    }

    double accum = 0.0;
    
    for (unsigned long long k = params->start_idx; k < params->end_idx; k++) {

        double d = k * 4.0 + 1.0;
        accum += (1.0 / d) - (1.0 / (d + 2.0));
    }

    *chunk_sum = accum;
    
    pthread_exit(chunk_sum);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "To use enter the number of threads!!\n");
        return EXIT_FAILURE;
    }

    int thread_count = atoi(argv[1]);
    if (thread_count <= 0) {
        fprintf(stderr, "Error not a valid positive number of threads.\n");
        return EXIT_FAILURE;
    }

    pthread_t* threads = malloc(thread_count * sizeof(pthread_t));
    ThreadParams* t_args = malloc(thread_count * sizeof(ThreadParams));

    if (!threads || !t_args) {
        perror("Error malloc in main");
        free(threads);
        free(t_args);
        return EXIT_FAILURE;
    }

    unsigned long long chunk_size = TOTAL_ITERATIONS / thread_count;
    unsigned long long remainder = TOTAL_ITERATIONS % thread_count;

    unsigned long long current_start = 0;
    for (int i = 0; i < thread_count; i++) {
        t_args[i].start_idx = current_start;
        
        unsigned long long my_chunk = chunk_size + (i < remainder ? 1 : 0);
        t_args[i].end_idx = current_start + my_chunk;
        
        current_start += my_chunk;

        int status = pthread_create(&threads[i], NULL, calculator, &t_args[i]);
        if (status != 0) {
            fprintf(stderr, "Error pthread_create for thread %d: %s\n", i, strerror(status));
            free(threads);
            free(t_args);
            return EXIT_FAILURE;
        }
    }

    double total_pi = 0.0;
    
    for (int i = 0; i < thread_count; i++) {
        void* returned_sum = NULL;
        
        int status = pthread_join(threads[i], &returned_sum);
        if (status != 0) {
            fprintf(stderr, "Error pthread_join for thread %d: %s\n", i, strerror(status));
            free(threads);
            free(t_args);
            return EXIT_FAILURE;
        }

        if (returned_sum) {
            total_pi += *(double *)returned_sum;
            free(returned_sum);
        }
    }

    total_pi *= 4.0;
    
    printf("Pi = %.15g\n", total_pi);

    free(threads);
    free(t_args);

    return EXIT_SUCCESS;
}