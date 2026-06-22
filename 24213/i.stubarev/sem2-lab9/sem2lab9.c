#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#define CHUNK_SIZE 1000000

volatile sig_atomic_t stop_flag = 0;
int shared_stop = 0;
int num_threads = 0;
pthread_barrier_t sync_barrier;

typedef struct {
    pthread_t thread;
    int thread_id;
    double partial_sum;
} ThreadContext;

void sigint_handler(int sig) {
    stop_flag = 1;
}

void* calc_pi(void* arg) {
    ThreadContext* ctx = (ThreadContext*)arg;
    ctx->partial_sum = 0.0;
    long long i = ctx->thread_id;

    while (1) {
        for (int k = 0; k < CHUNK_SIZE; k++) {
            ctx->partial_sum += 1.0 / (i * 4.0 + 1.0);
            ctx->partial_sum -= 1.0 / (i * 4.0 + 3.0);
            i += num_threads;
        }

        int b_res = pthread_barrier_wait(&sync_barrier);
        if (b_res != 0 && b_res != PTHREAD_BARRIER_SERIAL_THREAD) {
            fprintf(stderr, "Failed pthread_barrier_wait (1): %s\n", strerror(b_res));
            exit(1);
        }

        if (b_res == PTHREAD_BARRIER_SERIAL_THREAD) {
            shared_stop = stop_flag;
        }

        b_res = pthread_barrier_wait(&sync_barrier);
        if (b_res != 0 && b_res != PTHREAD_BARRIER_SERIAL_THREAD) {
            fprintf(stderr, "Failed pthread_barrier_wait (2): %s\n", strerror(b_res));
            exit(1);
        }

        if (shared_stop) {
            break;
        }
    }

    return arg;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "To run, write: %s <number of threads>\n", argv[0]);
        exit(1);
    }

    num_threads = atoi(argv[1]);
    if (num_threads < 1) {
        fprintf(stderr, "Number of threads must be a positive integer.\n");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Failed sigaction");
        exit(1);
    }

    int rc = pthread_barrier_init(&sync_barrier, NULL, num_threads);
    if (rc != 0) {
        fprintf(stderr, "Failed pthread_barrier_init: %s\n", strerror(rc));
        exit(1);
    }

    ThreadContext* contexts = malloc(num_threads * sizeof(ThreadContext));
    if (contexts == NULL) {
        perror("Failed malloc");
        pthread_barrier_destroy(&sync_barrier);
        exit(1);
    }

    for (int i = 0; i < num_threads; i++) {
        contexts[i].thread_id = i;
        rc = pthread_create(&contexts[i].thread, NULL, calc_pi, &contexts[i]);
        if (rc != 0) {
            fprintf(stderr, "Failed pthread_create: %s\n", strerror(rc));
            free(contexts);
            pthread_barrier_destroy(&sync_barrier);
            exit(1);
        }
    }

    double global_pi = 0.0;
    for (int i = 0; i < num_threads; i++) {
        ThreadContext* ret_ctx;
        rc = pthread_join(contexts[i].thread, (void**)&ret_ctx);
        if (rc == 0) {
            global_pi += ret_ctx->partial_sum;
        } else {
            fprintf(stderr, "Failed pthread_join: %s\n", strerror(rc));
            free(contexts);
            pthread_barrier_destroy(&sync_barrier);
            exit(1);
        }
    }

    global_pi *= 4.0;
    printf("\npi = %.15g \n", global_pi);

    free(contexts);
    pthread_barrier_destroy(&sync_barrier);
    exit(0);
}
