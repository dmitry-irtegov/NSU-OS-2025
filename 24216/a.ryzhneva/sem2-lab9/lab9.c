#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t stop_flag = 0;
int count_threads;

typedef struct DataThread_t {
    int idx;
    double value;
} DataThread;

void handle_sigint(int sig) {
    stop_flag = 1;
}

void* thread_body(void* param) {
    DataThread* intermed_data = (DataThread*)param;
    double partial_sum = 0.0;
    
    long long i = intermed_data->idx;
    
    while (!stop_flag) {
        for (int k = 0; k < 1000000; k++) {
            partial_sum += 1.0 / (i * 4.0 + 1.0);
            partial_sum -= 1.0 / (i * 4.0 + 3.0);
            i += count_threads;
        }
    }
    
    for (int k = 0; k < 50000000; k++) {
        partial_sum += 1.0 / (i * 4.0 + 1.0);
        partial_sum -= 1.0 / (i * 4.0 + 3.0);
        i += count_threads;
    }

    intermed_data->value = partial_sum; 
    pthread_exit(param);
}

void check_code(int code, const char* name_prog, const char* action) {
    if (code != 0) {
        char buf[256];
        strerror_r(code, buf, sizeof(buf));
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    int code;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <count_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    count_threads = atoi(argv[1]);
    if (count_threads < 1) {
        fprintf(stderr, "Error: Thread count must be at least 1.\n");
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        return EXIT_FAILURE;
    }

    pthread_t *threads = (pthread_t*)malloc(count_threads * sizeof(pthread_t));
    DataThread *intermed_data = (DataThread*)malloc(count_threads * sizeof(DataThread));

    if (!threads || !intermed_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    printf("Calculating Pi with %d threads. Press Ctrl+C to stop...\n", count_threads);

    for (int i = 0; i < count_threads; i++) {
        intermed_data[i].idx = i;
        intermed_data[i].value = 0.0;
        code = pthread_create(&threads[i], NULL, thread_body, (void*)&intermed_data[i]);
        check_code(code, argv[0], "creating thread");
    }

    double pi = 0.0;
    for (int i = 0; i < count_threads; i++) {
        DataThread *result_data;
        code = pthread_join(threads[i], (void**)&result_data);
        check_code(code, argv[0], "joining thread");
        pi += result_data->value;
    }

    pi = pi * 4.0;
    printf("\nPi calculation finished.\nEstimated value: %.15g\n", pi); 

    free(threads);
    free(intermed_data);
    
    return EXIT_SUCCESS;
}