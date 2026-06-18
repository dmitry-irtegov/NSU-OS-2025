#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h> 

/* Количество итераций может быть переопределено при компиляции через -DNUM_STEPS=... */
#ifndef NUM_STEPS
#define NUM_STEPS 200000000
#endif

typedef struct {
    int start;
    int end;
} thread_args_t;


void* calculate_pi_partial(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    double partial_sum = 0.0;

    for (int i = args->start; i < args->end; ++i) {
        partial_sum += 1.0 / (i * 4.0 + 1.0);
        partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }


    double* result = malloc(sizeof(double));
    if (result == NULL) {
        perror("malloc failed in thread");
        pthread_exit(NULL);
    }//чем отличается 10 по 1 и 1 на 10
    *result = partial_sum;

    pthread_exit((void*)result);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "использование: нужен параметр количества потоков\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "количество потоков должно быть положительным числом.\n");
        return EXIT_FAILURE;
    }

    int num_steps = NUM_STEPS;
    int chunk_size = num_steps / num_threads;


    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_args_t* args  = malloc(num_threads * sizeof(thread_args_t));

    if (threads == NULL || args == NULL) {
        perror("Memory allocation failed");
        return EXIT_FAILURE;
    }

    struct timespec t_start, t_end;  //время
    if (clock_gettime(CLOCK_MONOTONIC, &t_start) != 0) {
        perror("clock_gettime failed");
        return EXIT_FAILURE;
    }



    for (int t = 0; t < num_threads; ++t) {
        args[t].start = t * chunk_size;
        args[t].end   = (t == num_threads - 1) ? num_steps : (t + 1) * chunk_size;
        pthread_create(&threads[t], NULL, calculate_pi_partial, &args[t]);
    }

    double pi_quarter = 0.0;
    for (int t = 0; t < num_threads; ++t) {
        void* thread_result;
        pthread_join(threads[t], &thread_result);

        if (thread_result != NULL) {
            pi_quarter += *((double*)thread_result);
            free(thread_result);
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t_end) != 0) {
        perror("clock_gettime failed");
        return EXIT_FAILURE;
    }

    

    double pi = pi_quarter * 4.0;
    printf("pi done - %.15g\n", pi);
    
    double elapsed_sec = (t_end.tv_sec - t_start.tv_sec) +
                         (t_end.tv_nsec - t_start.tv_nsec) / 1000000000.0;
    printf("time      - %.6f sec\n", elapsed_sec);

    free(threads);
    free(args);

    return EXIT_SUCCESS;
}

//atomic operation