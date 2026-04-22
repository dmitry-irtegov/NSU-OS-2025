/* * File:   pi_pthreads_refactored.c
 * Multi-threaded calculation of Pi using Leibniz series.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define STEPS_AMOUNT 200000000

typedef struct {
    int start_ind;
    int end_ind;
} ThreadData;

void* calculate_partial_pi(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    double* part_sum = (double*)malloc(sizeof(double));
    if (part_sum == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in thread\n.");
        pthread_exit(NULL);
    }
    
    *part_sum = 0.0;
    
    for (int i = data->start_ind; i < data->end_ind; i++) {
         *part_sum += 1.0 / (i * 4.0 + 1.0);
         *part_sum -= 1.0 / (i * 4.0 + 3.0);
    }
    
    pthread_exit((void*)part_sum);
}

bool parse_args(int argc, char** argv, int* num_threads) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_of_threads>\n", argv[0]);
        return false;
    }
    
    *num_threads = atoi(argv[1]);
    if (*num_threads <= 0) {
        fprintf(stderr, "Error: Num of threads cannot be negative.\n");
        return false;
    }
    
    return true;
}

bool start_threads(int num_threads, pthread_t* threads, ThreadData* thread_args) {
    int steps_per_thread = STEPS_AMOUNT / num_threads;
    int remainder = STEPS_AMOUNT % num_threads;
    int curr_start = 0;

    for (int i = 0; i < num_threads; i++) {
        thread_args[i].start_ind = curr_start;
        thread_args[i].end_ind = curr_start + steps_per_thread + (i < remainder ? 1 : 0);
        curr_start = thread_args[i].end_ind;

        if (pthread_create(&threads[i], NULL, calculate_partial_pi, &thread_args[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread number %d\n", i);
            return false;
        }
    }
    return true;
}

bool collect_results(int num_threads, pthread_t* threads, double* total_pi) {
    *total_pi = 0.0;
    bool success = true;

    for (int i = 0; i < num_threads; i++) {
        double* returned_sum = NULL;
        
        if (pthread_join(threads[i], (void**)&returned_sum) != 0) {
            fprintf(stderr, "Error: Failed to join thread number %d\n", i);
            success = false;
            continue;
        }
        
        if (returned_sum != NULL) {
            *total_pi += *returned_sum;
            free(returned_sum);
        } else {
            fprintf(stderr, "Error: Thread %d returned a NULL pointer\n", i);
            success = false;
        }
    }
    
    *total_pi = *total_pi * 4.0;
    return success;
}

int main(int argc, char** argv) {
    int num_threads = 0;
    
    if (!parse_args(argc, argv, &num_threads)) {
        return EXIT_FAILURE;
    }

    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadData* thread_args = (ThreadData*)malloc(num_threads * sizeof(ThreadData));
    
    if (threads == NULL || thread_args == NULL) {
        fprintf(stderr, "Error: Memory allocation for threads structures failed in main\n");
        free(threads);
        free(thread_args);
        return EXIT_FAILURE;
    }

    if (!start_threads(num_threads, threads, thread_args)) {
        free(threads);
        free(thread_args);
        return EXIT_FAILURE;
    }

    double total_pi = 0.0;
    if (!collect_results(num_threads, threads, &total_pi)) {
        fprintf(stderr, "Warning: Pi value may be inaccurate.\n");
    }

    printf("pi result - %.15g \n", total_pi);    

    free(threads);
    free(thread_args);

    return EXIT_SUCCESS;
}