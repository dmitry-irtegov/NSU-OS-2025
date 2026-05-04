#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

#define NUM_STEPS 100000000

typedef struct {
    int thread_id;
    int thread_count;
} WorkerData;

static void *calculate_pi_part(void *data) {
    WorkerData *info = (WorkerData *)data;

    double *sum = malloc(sizeof(double));
    if (sum == NULL) {
        perror("malloc");
        pthread_exit(NULL);
    }

    *sum = 0.0;

    for (int k = info->thread_id; k < NUM_STEPS; k += info->thread_count) {
        *sum += 1.0 / (4.0 * k + 1.0);
        *sum -= 1.0 / (4.0 * k + 3.0);
    }

    return sum;
}

static int is_positive_number(const char *s) {
    if (s == NULL || *s == '\0') {
        return 0;
    }

    for (int i = 0; s[i] != '\0'; ++i) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <threads>\n", argv[0]);
        return 1;
    }

    if (!is_positive_number(argv[1])) {
        fprintf(stderr, "Error: invalid thread count: %s\n", argv[1]);
        return 1;
    }

    int thread_count = atoi(argv[1]);
    if (thread_count <= 0) {
        fprintf(stderr, "Error: thread count must be >0\n");
        return 1;
    }

    pthread_t *tid = malloc(thread_count * sizeof(pthread_t));
    WorkerData *workers = malloc(thread_count * sizeof(WorkerData));

    if (tid == NULL || workers == NULL) {
        perror("malloc");
        free(tid);
        free(workers);
        return 1;
    }

    for (int i = 0; i < thread_count; ++i) {
        workers[i].thread_id = i;
        workers[i].thread_count = thread_count;

        if (pthread_create(&tid[i], NULL, calculate_pi_part, &workers[i]) != 0) {
            perror("pthread_create");
            free(tid);
            free(workers);
            return 1;
        }
    }

    double pi = 0.0;

    for (int i = 0; i < thread_count; ++i) {
        double *partial = NULL;

        if (pthread_join(tid[i], (void **)&partial) != 0) {
            perror("pthread_join");
            free(tid);
            free(workers);
            return 1;
        }

        if (partial != NULL) {
            pi += *partial;
            free(partial);
        }
    }

    printf("pi done - %.15g\n", pi * 4.0);

    free(tid);
    free(workers);
    return 0;
}