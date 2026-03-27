#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_BLOCK 1000000LL

int stop_requested = 0;
unsigned long long global_max_iters = 0;
pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int thread_id;
    int num_threads;
    double final_sum;
    unsigned long long iters;
} thread_ctx_t;

void* signal_catcher_thread(void* arg) {
    sigset_t* wait_set = (sigset_t*)arg;
    int caught_signal;

    if (sigwait(wait_set, &caught_signal) == 0) {
        pthread_mutex_lock(&sync_mutex);
        stop_requested = 1;
        pthread_mutex_unlock(&sync_mutex);
    }
    return NULL;
}

void* calc_pi_part(void* arg) {
    thread_ctx_t* ctx = (thread_ctx_t*)arg;

    ctx->final_sum = 0.0;
    ctx->iters = 0;

    long long current_idx = ctx->thread_id;

    while (1) {
        double block_sum = 0.0;

        for (long long k = 0; k < WORK_BLOCK; k++) {
            block_sum += 1.0 / (current_idx * 4.0 + 1.0);
            block_sum -= 1.0 / (current_idx * 4.0 + 3.0);
            current_idx += ctx->num_threads;
        }

        ctx->final_sum += block_sum;
        ctx->iters += WORK_BLOCK;

        pthread_mutex_lock(&sync_mutex);

        if (ctx->iters > global_max_iters) {
            global_max_iters = ctx->iters;
        }

        int is_stopping = stop_requested;
        unsigned long long target_iters = global_max_iters;

        pthread_mutex_unlock(&sync_mutex);

        if (is_stopping && ctx->iters >= target_iters) {
            break;
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return 1;
    }

    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGINT);

    if (pthread_sigmask(SIG_BLOCK, &sig_mask, NULL) != 0) {
        perror("Error blocking signals");
        return 1;
    }

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    thread_ctx_t* ctx_array = malloc(num_threads * sizeof(thread_ctx_t));

    if (threads == NULL || ctx_array == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        free(threads);
        free(ctx_array);
        return 1;
    }

    pthread_t sig_thread;
    int error =
        pthread_create(&sig_thread, NULL, signal_catcher_thread, &sig_mask);
    if (error != 0) {
        fprintf(stderr, "Error creating signal thread: %s\n", strerror(error));
        return 1;
    }

    for (int i = 0; i < num_threads; i++) {
        ctx_array[i].thread_id = i;
        ctx_array[i].num_threads = num_threads;

        error = pthread_create(&threads[i], NULL, calc_pi_part, &ctx_array[i]);
        if (error != 0) {
            fprintf(stderr, "Error creating thread %d: %s\n", i,
                    strerror(error));
            free(threads);
            free(ctx_array);
            exit(1);
        }
    }

    printf(
        "Running with %d threads... press Ctrl+C to stop and print pi "
        "approximation.\n",
        num_threads);

    double pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "Error joining thread %d: %s\n", i,
                    strerror(error));
            exit(1);
        }

        pi += ctx_array[i].final_sum;
    }

    pi *= 4.0;
    printf("\npi done - %.15g\n", pi);

    free(threads);
    free(ctx_array);
    pthread_mutex_destroy(&sync_mutex);

    return 0;
}