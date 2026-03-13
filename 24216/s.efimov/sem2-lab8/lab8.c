#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define num_steps 200000000

struct thread_data {
    int start_index;
    int end_index;
};

void* thread_score(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    double* partial_sum = (double*)malloc(sizeof(double));
    *partial_sum = 0.0;
    for (int i = data->start_index; i < data->end_index; i++) {
        *partial_sum += 1.0/(i*4.0 + 1.0);
        *partial_sum -= 1.0/(i*4.0 + 3.0);
    }
    pthread_exit((void*)partial_sum);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Неверное количество аргументов.\n");
        return (EXIT_FAILURE);
    }
    char *endptr;
    long cntThreads = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || cntThreads <= 0) {
        fprintf(stderr, "Аргумент должен быть положительным целым числом.\n");
        return (EXIT_FAILURE);
    }
    double pi = 0;
    pthread_t threads[cntThreads];
    struct thread_data thread_data_array[cntThreads];
    for (int i = 0; i < cntThreads ; i++) {
        struct thread_data* data = &thread_data_array[i];
        data->start_index = i * (num_steps / cntThreads);
        if (i == cntThreads - 1) {
            data->end_index = num_steps;
        } else {
            data->end_index = (i + 1) * (num_steps / cntThreads);
        }
        int status = pthread_create(&threads[i], NULL, thread_score, (void*)data);
        if (status != 0) {
            fprintf(stderr, "Ошибка создания потока %d: %s\n", i, strerror(status));
            return (EXIT_FAILURE);
        }
    }
    for (int i = 0; i < cntThreads ; i++) {
        double* thread_result; 
        
        int status = pthread_join(threads[i], (void**)&thread_result);
        if (status != 0) {
            fprintf(stderr, "Ошибка join для потока %d: %s\n", i, strerror(status));
            return (EXIT_FAILURE);
        }
        pi += *thread_result;
        free(thread_result);
    }
    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);    
    return (EXIT_SUCCESS);
}
