#define _POSIX_C_SOURCE 199506L

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    const char **lines;
    int count;
    int id;
} ThreadData;

void *print_lines(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    for (int i = 0; i < data->count; i++) {
        flockfile(stdout);
        printf("Thread #%d: %s\n", data->id, data->lines[i]);
        funlockfile(stdout);
    }

    return NULL;
}

int main() {
    pthread_t threads[4];

    const char *seq1[] = {"a1", "aa2", "aaa3"};
    const char *seq2[] = {"b1", "bb2"};
    const char *seq3[] = {"c1", "cc2", "ccc3", "cccc4"};
    const char *seq4[] = {"d1"};

    ThreadData data[4] = {
        {seq1, 3, 1},
        {seq2, 2, 2},
        {seq3, 4, 3},
        {seq4, 1, 4}
    };

    for (int i = 0; i < 4; i++) {
        int rv = pthread_create(&threads[i], NULL, print_lines, &data[i]);
        if (rv != 0) {
            fprintf(stderr, "pthread_create failed (i=%d): %s\n", i, strerror(rv));
            return 1;
        }
    }

    for (int i = 0; i < 4; i++) {
        int rv = pthread_join(threads[i], NULL);
        if (rv != 0) {
            fprintf(stderr, "pthread_join failed (i=%d): %s\n", i, strerror(rv));
            return 1;
        }
    }

    return 0;
}
