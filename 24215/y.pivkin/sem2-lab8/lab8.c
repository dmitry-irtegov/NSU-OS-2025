#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define steps 200000000

typedef struct {
    int start;
    int end;
} thread_args_t;

void* pi_part(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    double sum = 0.0;
    int i;

    for (i = args->start; i < args->end; i++) {
        sum += 1.0 / (i * 4.0 + 1.0);
        sum -= 1.0 / (i * 4.0 + 3.0);
    }

    double* result = (double*)malloc(sizeof(double));
    if (result == NULL) {
        perror("malloc failed");
        pthread_exit(NULL);
    }

    *result = sum;
    pthread_exit((void*)result);
}



int main(int argc, char** argv) {
    int threads_count;
    pthread_t* threads;
    thread_args_t* thread_args;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [threads_count]\n", argv[0]);
        exit(1);
    }

    threads_count = atoi(argv[1]);
    if (threads_count <= 0) {
        fprintf(stderr, "Number of threads must be positive.\n");
        exit(2);
    }

    threads = (pthread_t*)malloc(threads_count * sizeof(pthread_t));
    if (threads == NULL) {
        perror("malloc");
        exit(3);
    }

    thread_args = (thread_args_t*)malloc(threads_count * sizeof(thread_args_t));
    if (thread_args == NULL) {
        perror("malloc");
        exit(4);
    }



    int part = steps / threads_count;
    int rest = steps % threads_count;
    double pi = 0.0;
    int i;

    for (i = 0; i < threads_count; i++) {
        thread_args[i].start = part * i;
        thread_args[i].end   = part * (i + 1);

        if (i < rest) {
            thread_args[i].start += i;
            thread_args[i].end   += i + 1;
        } else {
            thread_args[i].start += rest;
            thread_args[i].end   += rest;
        }

        if (pthread_create(&threads[i], NULL, pi_part, (void*)&thread_args[i]) != 0) {
            perror("pthread_create");
            exit(5);
        }
    }



    for (i = 0; i < threads_count; i++) {
        void* ptr;

        if (pthread_join(threads[i], &ptr) != 0) {
            perror("pthread_join");
            exit(6);
        }

        if (ptr != NULL) {
            double sum = *(double*)ptr;
            pi += sum;

            free(ptr);
        } else {
            fprintf(stderr, "Thread returned strange value.\n");
            exit(7);
        }
    }

    pi = pi * 4.0;

    printf("%.15g - computed with %d threads.\n", pi, threads_count);
    printf("3.14159265358979 - value from wiki.\n");

    free(threads);
    free(thread_args);

    exit(0);
}
