#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef NUM_STEPS
#define NUM_STEPS 1000000000
#endif

typedef struct {
    int thread_id;
    int num_threads;
} thread_data_t;

void* calculate_pi_part(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    double* partial_sum = malloc(sizeof(double));
    *partial_sum = 0.0;

    for (int i = data->thread_id; i < NUM_STEPS; i += data->num_threads) {
        *partial_sum += 1.0 / (i * 4.0 + 1.0);
        *partial_sum -= 1.0 / (i * 4.0 + 3.0);
    }

    pthread_exit(partial_sum);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "%s <количество_потоков>\n", argv[0]);
        return 1;
    }

    char *endptr;
    long num_threads_long = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || num_threads_long <= 0) {
        fprintf(stderr, "Ошибка: некорректное количество потоков. Введите положительное целое число (1-10000).\n");
        return 1;
    }
    int num_threads = (int)num_threads_long;
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        if (pthread_create(&threads[i], NULL, calculate_pi_part, &thread_data[i]) != 0) {
            fprintf(stderr, "Ошибка: не удалось создать поток %d\n", i);
            for (int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
            }
            return 1;
        }
    }

    double total_pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        double* part;
        pthread_join(threads[i], (void**)&part);
        total_pi += *part;
        free(part);
    }

    printf("pi done - %.15g\n", total_pi * 4.0);

    return 0;
}
