#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_STEPS 200000000

typedef struct {
    long start;
    long end;
} ThreadData;

void* child_thread(void* arg)
{
    ThreadData* data = (ThreadData*)arg;

    double* part_sum = malloc(sizeof(double));
    if (part_sum == NULL) {
        perror("malloc failed");
        pthread_exit(NULL);
    }

    *part_sum = 0.0;
    for (long i = data->start; i < data->end; i++) {
        *part_sum += 1.0 / (4.0 * i + 1.0);
        *part_sum -= 1.0 / (4.0 * i + 3.0);
    }

    pthread_exit(part_sum);
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <threads_count>\n", argv[0]);
        return 1;
    }

    int threads_count = atoi(argv[1]);
    if (threads_count <= 0) {
        fprintf(stderr, "Number of threads must be > 0\n");
        return 1;
    }

    pthread_t* threads = malloc(threads_count * sizeof(pthread_t));
    ThreadData* data = malloc(threads_count * sizeof(ThreadData));
    if (threads == NULL || data == NULL) {
        perror("malloc failed");
        free(threads);
        free(data);
        return 1;
    }

    long chunk = NUM_STEPS / threads_count;

    for (int i = 0; i < threads_count; i++) {
        data[i].start = i * chunk;

        if (i == threads_count - 1)
            data[i].end = NUM_STEPS;
        else
            data[i].end = (i + 1) * chunk;

        int ret = pthread_create(&threads[i], NULL, child_thread, &data[i]);
        if (ret != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", ret);
            free(threads);
            free(data);
            return 1;
        }
    }

    double pi = 0.0;

    for (int i = 0; i < threads_count; i++) {

        void* ret_part_sum;
        int ret = pthread_join(threads[i], &ret_part_sum);

        if (ret != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", ret);
            free(threads);
            free(data);
            return 1;
        }

        if (ret_part_sum != NULL) {
            pi += *((double*)ret_part_sum);
            free(ret_part_sum);
        }
    }

    pi *= 4.0;
    printf("pi - %.15f\n", pi);

    free(threads);
    free(data);
    return 0;
}