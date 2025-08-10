#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS_COUNT 4

char *thread_0_lines[] = {"0", "another 0", "another 0", "enough 0"};
char *thread_1_lines[] = {"enough 1"};
char *thread_2_lines[] = {"2", "another 2", "enough 2"};
char *thread_3_lines[] = {"3", "enough 3"};

typedef struct {
    char **lines;
    int count;
} Data;

void *print(void *arg) {
    Data *data = (Data *)arg;
    for (int j = 0; j < data->count; j++) {
        printf("%s\n", data->lines[j]);
    }
    return NULL;
}

int main() {
    pthread_t threads[THREADS_COUNT];
    Data threads_data[THREADS_COUNT];
    int rc;

    threads_data[0].lines = thread_0_lines;
    threads_data[0].count = sizeof(thread_0_lines) / sizeof(thread_0_lines[0]);;
    threads_data[1].lines = thread_1_lines;
    threads_data[1].count = sizeof(thread_1_lines) / sizeof(thread_1_lines[0]);;
    threads_data[2].lines = thread_2_lines;
    threads_data[2].count = sizeof(thread_2_lines) / sizeof(thread_2_lines[0]);;
    threads_data[3].lines = thread_3_lines;
    threads_data[3].count = sizeof(thread_3_lines) / sizeof(thread_3_lines[0]);;

    for (int i = 0; i < THREADS_COUNT; i++) {
        rc = pthread_create(&threads[i], NULL, print, &threads_data[i]);
        if (rc != 0) {
            char buf[256];
            strerror_r(rc, buf, sizeof buf);
            fprintf(stderr, "creating thread: %s\n", buf);
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < THREADS_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    exit(EXIT_SUCCESS);
}
