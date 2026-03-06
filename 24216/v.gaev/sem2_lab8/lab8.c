#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define num_steps 200000000

typedef struct {
    int start_idx;
    int end_idx;
} thread_args_t;

void* calculate_pi_partial(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    
    double* partial_sum = (double*)malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("Ошибка выделения памяти для частичной суммы");
        pthread_exit(NULL);
    }
    
    *partial_sum = 0.0;
    
    for (int i = args->start_idx; i < args->end_idx; i++) {
         *partial_sum += 1.0 / (i * 4.0 + 1.0);
         *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }
    
    pthread_exit((void*)partial_sum);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <количество_потоков>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Количество потоков должно быть положительным числом.\n");
        return EXIT_FAILURE;
    }

    if (num_threads > num_steps) {
        num_threads = num_steps;
    }

    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_args_t* args = (thread_args_t*)malloc(num_threads * sizeof(thread_args_t));
    
    if (threads == NULL || args == NULL) {
        perror("Ошибка выделения памяти");
        return EXIT_FAILURE;
    }

    int steps_per_thread = num_steps / num_threads;
    int remainder = num_steps % num_threads;
    int current_start = 0;

    for (int i = 0; i < num_threads; i++) {
        args[i].start_idx = current_start;
        int extra_step = (i < remainder) ? 1 : 0;
        args[i].end_idx = current_start + steps_per_thread + extra_step;
        current_start = args[i].end_idx;

        if (pthread_create(&threads[i], NULL, calculate_pi_partial, &args[i]) != 0) {
            perror("Ошибка при создании потока");
            return EXIT_FAILURE;
        }
    }

    double pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        double* partial_sum;
        
        if (pthread_join(threads[i], (void**)&partial_sum) != 0) {
            perror("Ошибка при ожидании потока");
            return EXIT_FAILURE;
        }
        
        if (partial_sum != NULL) {
            pi += *partial_sum;
            free(partial_sum);
        }
    }

    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);    

    free(threads);
    free(args);

    return EXIT_SUCCESS;
}
