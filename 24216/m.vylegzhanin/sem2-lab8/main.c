#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef num_steps
#define num_steps 200000000LL
#endif

struct thread_arg {
    long tid;
    long nthreads;
};

void *thread_func(void *arg) {
    struct thread_arg *t = (struct thread_arg *)arg;
    long id = t->tid;
    long nthreads = t->nthreads;

    long long base = (long long)num_steps / nthreads;
    long long rem = (long long)num_steps % nthreads;
    long long start = id * base + (id < rem ? id : rem);
    long long end = start + base + (id < rem ? 1 : 0);

    double local_sum = 0.0;
    for (long long i = start; i < end; ++i) {
        local_sum += 1.0 / (4.0 * (double)i + 1.0);
        local_sum -= 1.0 / (4.0 * (double)i + 3.0);
    }

    double *ret = malloc(sizeof(double));
    if (ret == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }
    *ret = local_sum * 4.0;
    pthread_exit(ret);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <nthreads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long nthreads = strtol(argv[1], NULL, 10);
    if (nthreads <= 0) {
        fprintf(stderr, "nthreads must be > 0\n");
        return EXIT_FAILURE;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
    struct thread_arg *args = malloc(sizeof(struct thread_arg) * nthreads);
    if (threads == NULL || args == NULL) {
        perror("malloc");
        free(threads);
        free(args);
        return EXIT_FAILURE;
    }

    for (long i = 0; i < nthreads; ++i) {
        args[i].tid = i;
        args[i].nthreads = nthreads;
        int rc = pthread_create(&threads[i], NULL, thread_func, &args[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", rc);
            free(threads);
            free(args);
            return EXIT_FAILURE;
        }
    }

    double pi = 0.0;
    for (long i = 0; i < nthreads; ++i) {
        void *rval;
        int rc = pthread_join(threads[i], &rval);
        if (rc != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", rc);
            free(threads);
            free(args);
            return EXIT_FAILURE;
        }
        if (rval != NULL) {
            double *partial = (double *)rval;
            pi += *partial;
            free(partial);
        }
    }

    printf("pi done - %.15g\n", pi);

    free(threads);
    free(args);
    return EXIT_SUCCESS;
}
