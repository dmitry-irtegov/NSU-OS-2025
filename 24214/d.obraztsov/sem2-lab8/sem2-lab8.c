#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_STEPS 200000000

typedef struct {
    unsigned long long start;
    unsigned long long end;
    double *result;
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg* data = (ThreadArg*)arg;
    double sum = 0.0;
    
    for (unsigned long long i = data->start; i < data->end; i++) {
        sum += 1.0 / (4.0 * (double)i + 1.0);
        sum -= 1.0 / (4.0 * (double)i + 3.0);
    }
    
    *data->result = sum;
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Error: number of threads must be positive.\n");
        return 1;
    }
    
    const unsigned long long total_iter = NUM_STEPS;
    const unsigned long long chunk = total_iter / num_threads;
    const unsigned long long remainder = total_iter % num_threads;
    
    double* results = (double*)malloc(num_threads * sizeof(double));
    if (!results) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        fprintf(stderr, "Memory allocation failed\n");
        free(results);
        return 1;
    }
    
    ThreadArg* args_list = (ThreadArg*)malloc(num_threads * sizeof(ThreadArg));
    if (!args_list) {
        fprintf(stderr, "Memory allocation failed\n");
        free(results);
        free(threads);
        return 1;
    }
    
    unsigned long long current_start = 0;
    
    for (int t = 0; t < num_threads; t++) {
        unsigned long long chunk_size = chunk + ((unsigned long long) t < remainder ? 1 : 0);
        
        args_list[t].start = current_start;
        args_list[t].end = current_start + chunk_size;
        args_list[t].result = &results[t];
        current_start += chunk_size;
        
        if (pthread_create(&threads[t], NULL, thread_func, &args_list[t]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", t);
            free(results);
            free(threads);
            free(args_list);
            return 1;
        }
    }
    
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
    
    double total = 0.0;
    for (int t = 0; t < num_threads; t++) {
        total += results[t];
    }
    
    double pi = total * 4.0;
    printf("pi done - %.15f\n", pi);
    
    free(results);
    free(threads);
    free(args_list);
    
    return 0;
}