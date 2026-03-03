#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NUM_STEPS 200000000

typedef struct {
    int start;
    int end;
    double partial_sum;
} thread_data_t;

void* calculate_partial_pi(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    double sum = 0.0;
    
    for (int i = data->start; i < data->end; i++) {
        sum += 1.0 / (i * 4.0 + 1.0);
        sum -= 1.0 / (i * 4.0 + 3.0);
    }
    
    data->partial_sum = sum;
    
    return data;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return EXIT_FAILURE;
    }
    
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(num_threads * sizeof(thread_data_t));
    
    if (threads == NULL || thread_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        free(threads);
        free(thread_data);
        return EXIT_FAILURE;
    }
    int steps_per_thread = NUM_STEPS / num_threads;
    int remainder = NUM_STEPS % num_threads;
    int current_start = 0;
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + steps_per_thread;
        
        if (i < remainder) {
            thread_data[i].end++;
        }
        
        current_start = thread_data[i].end;
        
        int err = pthread_create(&threads[i], NULL, calculate_partial_pi, &thread_data[i]);
        if (err != 0) {
            fprintf(stderr, "Failed to create thread %d: %s\n", i, strerror(err));
            free(threads);
            free(thread_data);
            exit(EXIT_FAILURE);
        }
    }
    
    double pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        thread_data_t* result;
        
        int err = pthread_join(threads[i], (void**)&result);
        if (err != 0) {
            fprintf(stderr, "Failed to join thread %d: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        } else {
            pi += result->partial_sum;
        }
    }
    
    pi = pi * 4.0;
    printf("pi = %.15g\n", pi);
    
    free(threads);
    free(thread_data);
    
    return EXIT_SUCCESS;
}
