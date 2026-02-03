#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define THREADS_NUM 4

typedef struct {
    char **strings;
    int count;
} ThreadData;

void *print_strings(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    for (int i = 0; i < data->count; i++) {
        printf("%s\n", data->strings[i]);
    }

    return NULL;
}

int main() {
    pthread_t threads[THREADS_NUM];
    ThreadData thread_data[THREADS_NUM];

    char *seq1[] = {"1.1", "1.2", "1.3"};
    thread_data[0].strings = seq1;
    thread_data[0].count = 3;

    char *seq2[] = {"2.1", "2.2", "2.3", "2.4"};
    thread_data[1].strings = seq2;
    thread_data[1].count = 4;

    char *seq3[] = {"3.1", "3.2", "3.3", "3.4"};
    thread_data[2].strings = seq3;
    thread_data[2].count = 4;

    char *seq4[] = {"4.1", "4.2"};
    thread_data[3].strings = seq4;
    thread_data[3].count = 2;

    int code;
    for (int i = 0; i < THREADS_NUM; i++) {
        code = pthread_create(&threads[i], NULL, print_strings, &thread_data[i]);
        if (code != 0) {
            char buf[256];
            strerror_r(code, buf, sizeof buf);
            fprintf(stderr, "Thread creation error: %s\n", buf);
            exit(1);
        }
    }

    for (int i = 0; i < THREADS_NUM; i++) {
        code = pthread_join(threads[i], NULL);
        if (code != 0) {
            char buf[256];
            strerror_r(code, buf, sizeof buf);
            fprintf(stderr, "Thread join error: %s\n", buf);
            exit(1);
        }
    }

    exit(0);
}
