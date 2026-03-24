#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define ITER 20000000000
#define MAX_THREADS 50

typedef struct {
    double sum;
    long long startIter;
    long long endIter;
    int count;
} thread_data;

void* thread_body(void* arg) {
    thread_data* data = (thread_data*)arg;
    double curSum = 0.0;

    for (long long i = data->startIter; i < data->endIter; i += data->count) {
        curSum += 1.0 / (i * 4.0 + 1.0);
        curSum -= 1.0 / (i * 4.0 + 3.0);
    }
    data->sum = curSum;
    return (void*)data;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "incorrect number of arguments\n");
        return -1;
    }
    int countThreads = atoi(argv[1]);
    if (countThreads <= 0 || countThreads > MAX_THREADS) {
        fprintf(stderr, "the number of threads should be between 1 and %d\n", MAX_THREADS);
        return -1;
    }

    pthread_t threads[MAX_THREADS];
    thread_data dataThreads[MAX_THREADS];

    for (int i = 0; i < countThreads; i++) {
        dataThreads[i].startIter = i;
        dataThreads[i].endIter = ITER;
        dataThreads[i].sum = 0;
        dataThreads[i].count = countThreads;
        int code = pthread_create(&threads[i], NULL, thread_body, (void*)&dataThreads[i]);
        if (code != 0) {
            fprintf(stderr, "creating thread: %s\n", strerror(code));
            return -1;
        }
    }

    double result = .0;
    for (int i = 0; i < countThreads; i++) {
        thread_data* curData;
        pthread_join(threads[i], (void**)&curData);
        result += curData->sum;
    }
    result *= 4.0;

    printf("%.15g\n", result);
    return 0;
}
