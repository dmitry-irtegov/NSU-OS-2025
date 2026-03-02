#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define ITERS_CHECK 1000000

volatile sig_atomic_t keep_running = 1;
int num_threads;

typedef struct {
    int id;
    double part_sum;
} thread_data;

void sigint_handler(int sigNum) {
    const char msg[] = "\nCtrl+C received. Stopping...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
}

void* pi_count(void *arg) {
    thread_data *data = (thread_data*)arg;
    double part_pi = 0.0;
    long long i = data->id;

    while (keep_running) {
        for (long long step = 0; step < ITERS_CHECK; step++) {
            part_pi += 1.0 / (i * 4.0 + 1.0);
            part_pi -= 1.0 / (i * 4.0 + 3.0);
            i += num_threads;
        }
    }
    
    data->part_sum = part_pi;
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return -1;
    }

    num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be > 0.\n");
        return -1;
    }

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        return -1;
    }

    thread_data* threads_data = (thread_data*)malloc(num_threads * sizeof(thread_data));
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));

    if (!threads_data || !threads) {
        perror("malloc");
        return -1;
    }

    for (int i = 0; i < num_threads; i++) {
        threads_data[i].id = i;
        threads_data[i].part_sum = 0;
        if (pthread_create(&threads[i], NULL, pi_count, (void*)&threads_data[i]) != 0) {
            perror("pthread_create");
            return -1;
        }  
    }

    double pi = 0.0;

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        pi += threads_data[i].part_sum;
    }

    pi = pi * 4.0;
    printf("pi done - %.15g\n", pi);  
    
    free(threads);
    free(threads_data);  
    return 0;
}