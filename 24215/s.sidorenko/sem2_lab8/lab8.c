#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct thread_arg_t {
    long thread_id;
    long num_threads;
} thread_arg;

static void *compute_sum(void *arg) {
    thread_arg *targ = (thread_arg*)arg;
    long id = targ->thread_id;
    long nthreads = targ->num_threads;
    double sum = 0.0;
    long i;

    for (i = id; i < 200000000; i += nthreads) {
        sum += 1.0 / (i * 4.0 + 1.0);
        sum -= 1.0 / (i * 4.0 + 3.0);
    }

    double *result = (double*)malloc(sizeof(double));
    if (result == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }
    *result = sum;
    pthread_exit((void*)result);
}

int main(int argc, char *argv[]) {
    long num_threads = strtol(argv[1], NULL, 10);

    pthread_t *threads = (pthread_t*)malloc(num_threads* sizeof(pthread_t));
    thread_arg *args = (thread_arg*)malloc(num_threads* sizeof(thread_arg));

    if (threads == NULL || args == NULL) { 
        perror("malloc");
        return 1;
    }

    long t;
    for (size_t t = 0; t < num_threads; t++) {
        args[t].thread_id = t;
        args[t].num_threads = num_threads;

        int td = pthread_create(&threads[t], NULL, compute_sum, &args[t]);
        if (td != 0) {
            fprintf(stderr, "pthread_create: thread %ld\n", t);
            return 1;
        }
    }

    double pi = 0.0;
    for (t = 0; t < num_threads; t++) {
        void *val = NULL;
        int td = pthread_join(threads[t], &val);
        if (td != 0) {
            fprintf(stderr, "pthread_join: thread %ld\n", t);
            return 1;
        }
        if (val != NULL) {
            double *partial = (double *)val;
            pi += *partial;
            free(partial);
        }
    }

    pi *= 4.0;
    printf("pi = %.15g\n", pi);

    free(threads);
    free(args);
    return 0;
}