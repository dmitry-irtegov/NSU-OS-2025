#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define num_steps 2000000000


typedef struct {
    int start_step;
    int end_step;
} thread_args_t;

void* calculate_pi_chunk(void* arg) {
    thread_args_t* t_args = (thread_args_t*)arg;

    double* partial_sum = (double*)malloc(sizeof(double));
    if (partial_sum == NULL) {
        perror("Ошибка malloc внутри потока");
        pthread_exit(NULL);
    }

    *partial_sum = 0.0;


    for (int i = t_args->start_step; i < t_args->end_step; i++) {
         *partial_sum += 1.0/(i*4.0 + 1.0);
         *partial_sum -= 1.0/(i*4.0 + 3.0);
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
        fprintf(stderr, "Количество потоков должно быть больше 0.\n");
        return EXIT_FAILURE;
    }

    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_args_t* args = (thread_args_t*)malloc(num_threads * sizeof(thread_args_t));
    if (!threads || !args) {
        perror("Ошибка выделения памяти в главном потоке");
        return EXIT_FAILURE;
    }

    int steps_per_thread = num_steps / num_threads;
    int remainder = num_steps % num_threads;
    int current_start = 0;


    for (int i = 0; i < num_threads; i++) {
        args[i].start_step = current_start;
        args[i].end_step = current_start + steps_per_thread + (i < remainder ? 1 : 0);
        current_start = args[i].end_step;

        if (pthread_create(&threads[i], NULL, calculate_pi_chunk, (void*)&args[i]) != 0) {
            perror("Ошибка создания потока");
            return EXIT_FAILURE;
        }
    }

    double pi = 0.0;


    for (int i = 0; i < num_threads; i++) {
        double* ret_val;
        if (pthread_join(threads[i], (void**)&ret_val) != 0) {
            perror("Ошибка присоединения потока");
            return EXIT_FAILURE;
        }

        if (ret_val != NULL) {
            pi += *ret_val;
            free(ret_val);
        }
    }

    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);
    free(threads);
    free(args);

    return EXIT_SUCCESS;
}
