#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

int stop_flag = 0;
pthread_mutex_t flag_mutex = PTHREAD_MUTEX_INITIALIZER;
int count_threads;

typedef struct DataThread_t {
    int idx;
    double value;
} DataThread;

void* signal_thread_func(void* arg) {
    sigset_t* set = (sigset_t*)arg;
    int sig;
    
    if (sigwait(set, &sig) != 0) {
        perror("sigwait failed");
        pthread_exit(NULL);
    }
    
    pthread_mutex_lock(&flag_mutex);
    stop_flag = 1;
    pthread_mutex_unlock(&flag_mutex);
    
    return NULL;
}

void* thread_body(void* param) {
    DataThread* intermed_data = (DataThread*)param;
    double partial_sum = 0.0;
    long long i = intermed_data->idx;
    int local_stop = 0;
    
    while (!local_stop) {
        for (int k = 0; k < 1000000; k++) {
            partial_sum += 1.0 / (i * 4.0 + 1.0);
            partial_sum -= 1.0 / (i * 4.0 + 3.0);
            i += count_threads;
        }
        
        pthread_mutex_lock(&flag_mutex);
        local_stop = stop_flag;
        pthread_mutex_unlock(&flag_mutex);
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

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask failed");
        return EXIT_FAILURE;
    }

    pthread_t *threads = (pthread_t*)malloc(count_threads * sizeof(pthread_t));
    DataThread *intermed_data = (DataThread*)malloc(count_threads * sizeof(DataThread));
    pthread_t sig_thread;

    if (!threads || !intermed_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    code = pthread_create(&sig_thread, NULL, signal_thread_func, (void*)&set);
    check_code(code, argv[0], "creating signal thread");

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
    pthread_mutex_destroy(&flag_mutex);
    
    return EXIT_SUCCESS;
}