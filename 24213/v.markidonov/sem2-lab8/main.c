#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    long long start;
    long long end;
    int decrement;
    double sum;
} calc_request;

void* calculate(void* calc_req) {
    calc_request* request = (calc_request*)calc_req;

    for (long long i = request->start; i >= request->end; i -= request->decrement) {
        request->sum += 1.0 / (i * 4.0 + 1.0);
        request->sum -= 1.0 / (i * 4.0 + 3.0);
    }

    return calc_req;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <steps> <threads>\n", argv[0]);
        return -1;
    }

    long long steps = atoll(argv[1]);
    if (steps <= 0) {
        fprintf(stderr, "incorrect number of steps\n");
        return -1;
    }

    int threads_count = atoi(argv[2]);
    if (threads_count <= 0) {
        fprintf(stderr, "incorrect number of threads\n");
        return -1;
    }

    int code = 0;
    pthread_t threads[threads_count];
    calc_request calc_requests[threads_count];

    for (int i = 0; i < threads_count; i++) {
        calc_requests[i].start = steps - i;
        calc_requests[i].end = 0;
        calc_requests[i].decrement = threads_count;
        calc_requests[i].sum = 0.0;
        code = pthread_create(&threads[i], NULL, calculate, (void*)&calc_requests[i]);
        if (code != 0) {
            fprintf(stderr, "creating thread: %s\n", strerror(code));
            return -1;
        }
    }

    double result = 0.0;
    for (int i = 0; i < threads_count; i++) {
        calc_request *calc_response;
        code = pthread_join(threads[i], (void**)&calc_response);
        if (code != 0) {
            fprintf(stderr, "joining thread: %s\n", strerror(code));
            return -1;
        }
        result += calc_response->sum;
    }
    result *= 4.0;

    printf("%.15g\n", result);
    return 0;
}
