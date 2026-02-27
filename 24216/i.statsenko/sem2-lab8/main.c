#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef num_steps
#define num_steps 200000000LL
#endif

typedef struct {
    long long start;
    long long end;
} ThreadData;

void *threadComputer(void *data) {
    ThreadData *threadData = (ThreadData *)data;
    long long start = threadData->start;
    long long end = threadData->end;
    double *partialSum = malloc(sizeof(double));
    if (partialSum == NULL) {
        perror("malloc in threadComputer");
        pthread_exit(NULL);
    }
    *partialSum = 0.0;

    for (long long i = start; i < end; i++) {
        *partialSum += 1.0 / (i * 4.0 + 1.0);
        *partialSum -= 1.0 / (i * 4.0 + 3.0);
    }
    pthread_exit(partialSum);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number of threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *endptr;
    long countThread = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || countThread <= 0) {
        fprintf(stderr, "Number of threads must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    pthread_t *threads = malloc(countThread * sizeof(pthread_t));
    if (threads == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    ThreadData *dataArray = malloc(countThread * sizeof(ThreadData));
    if (dataArray == NULL) {
        perror("malloc");
        free(threads);
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < countThread; i++) {
        dataArray[i].start = i * (num_steps / countThread);
        if (i == countThread - 1) {
            dataArray[i].end = num_steps;
        } else {
            dataArray[i].end = (i + 1) * (num_steps / countThread);
        }
        int err = pthread_create(&threads[i], NULL, threadComputer, &dataArray[i]);
        if (err != 0) {
            fprintf(stderr, "Error creating thread %ld: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        }
    }

    double pi = 0.0;
    for (long i = 0; i < countThread; i++) {
        void *partialSum;
        int err = pthread_join(threads[i], &partialSum);
        if (err != 0) {
            fprintf(stderr, "Error joining thread %ld: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        }
        if (partialSum == NULL) {
            fprintf(stderr, "Thread %ld returned NULL partial sum\n", i);
            exit(EXIT_FAILURE);
        }
        pi += *(double *)partialSum;
        free(partialSum);
    }
    pi *= 4.0;
    fprintf(stderr, "Pi = %.15g\n", pi);
    free(threads);
    free(dataArray);
    return 0;
}