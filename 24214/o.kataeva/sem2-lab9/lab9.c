#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define ITERS_CHECK 1000000

volatile sig_atomic_t keep_running = 1;
int agreed_stop = 0;
int num_threads;
pthread_barrier_t end_barrier;

typedef struct {
    int id;
    double part_sum;
} thread_data;

void sigint_handler(int sigNum) {
    (void)sigNum;
    const char msg[] = "\nCtrl+C received. Stopping...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
}

void* pi_count(void *arg) {
    thread_data *data = (thread_data*)arg;
    double part_pi = 0.0;
    long long i = data->id;
    long long iterations = 0;

    while (1) {
        part_pi += 1.0 / (i * 4.0 + 1.0);
        part_pi -= 1.0 / (i * 4.0 + 3.0);
        
        i += num_threads;
        iterations++;

        if (iterations == ITERS_CHECK) {
            iterations = 0;
            int rc = pthread_barrier_wait(&end_barrier);
            if (rc == PTHREAD_BARRIER_SERIAL_THREAD) {
                if (!keep_running) {
                    agreed_stop = 1;
                }
            }
            pthread_barrier_wait(&end_barrier);

            if (agreed_stop) {
                break;
            }
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

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    if (pthread_barrier_init(&end_barrier, NULL, (unsigned)num_threads) != 0) {
        perror("pthread_barrier_init");
        return -1;
    }

    sigset_t set, old_set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &set, &old_set) != 0) {
        perror("pthread_sigmask");
        pthread_barrier_destroy(&end_barrier);
        return -1;
    }

    thread_data* threads_data = (thread_data*)malloc(num_threads * sizeof(thread_data));
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));

    if (!threads_data || !threads) {
        perror("malloc");
        free(threads_data);
        free(threads);
        pthread_barrier_destroy(&end_barrier);
        return -1;
    }

    int threads_created = 0;
    for (int i = 0; i < num_threads; i++) {
        threads_data[i].id = i;
        threads_data[i].part_sum = 0;
        if (pthread_create(&threads[i], NULL, pi_count, (void*)&threads_data[i]) != 0) {
            perror("pthread_create");
            free(threads_data);
            free(threads);
            pthread_barrier_destroy(&end_barrier);
            return -1;
        }
        threads_created++;
    }

    pthread_sigmask(SIG_SETMASK, &old_set, NULL);

    double pi = 0.0;

    for (int i = 0; i < threads_created; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err == 0) {
            pi += threads_data[i].part_sum;
        } else {
            fprintf(stderr, "pthread_join failed for thread %d: %s\n", i, strerror(err));
        }
    }

    pi = pi * 4.0;
    printf("pi done - %.15g\n", pi);  
    
    free(threads);
    free(threads_data);  
    pthread_barrier_destroy(&end_barrier);
    return 0;
}