#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define num_steps 200000000

typedef struct {
    int thread_id;
    int num_threads;
} thread_data_t;

void *calc_pi(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int id = data->thread_id;
    int n_threads = data->num_threads;
    
    double *partial_sum = (double *)malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("malloc error");
        pthread_exit(NULL);
    }
    *partial_sum = 0.0;
    
    int chunk_size = num_steps / n_threads;
    int start = id * chunk_size;
    int end = (id == n_threads - 1) ? num_steps : start + chunk_size;
    
    for (int i = start; i < end; i++) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }
    
    pthread_exit((void *)partial_sum);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <количество потоков>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Количество потоков должно быть положительным числом.\n");
        return EXIT_FAILURE;
    }
    
    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    thread_data_t *thread_data = (thread_data_t *)malloc(num_threads * sizeof(thread_data_t));
    
    if (threads == NULL || thread_data == NULL) {
        perror("malloc error");
        return EXIT_FAILURE;
    }
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        if (pthread_create(&threads[i], NULL, calc_pi, &thread_data[i]) != 0) {
            perror("pthread_create error");
            return EXIT_FAILURE;
        }
    }
    
    double pi = 0.0;
    
    for (int i = 0; i < num_threads; i++) {
        void *status;
        if (pthread_join(threads[i], &status) != 0) {
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
        
        if (status != NULL) {
            double *sum_from_thread = (double *)status;
            pi += *sum_from_thread;
            free(sum_from_thread);
        }
    }
    
    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);
    
    free(threads);
    free(thread_data);
    
    return EXIT_SUCCESS;
}
