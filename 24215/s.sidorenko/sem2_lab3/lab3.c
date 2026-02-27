#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct ThreadStruct_t {
    const char **lines;
    int threadCount;
    int len;
} ThreadStruct;

void *print_lines(void *arg) {
    ThreadStruct *lines = (ThreadStruct *)arg;
    int chunk_size = lines->len / 4;

    int start = lines->threadCount * chunk_size;

    if (lines->threadCount == 3) {
        for (int i = start; i < lines->len; i++) {
            printf("%s", lines->lines[i]);
        }
    } else {
        for (int i = start; i < start + chunk_size; i++) {
            printf("%s", lines->lines[i]);
        }
    }
    return NULL;
}

int main() {
    pthread_t threads[4];
    ThreadStruct args[4];

    const char *strings[] = {"thread 1: 1\n", "thread 1: 2\n", "thread 1: 3\n",
    "thread 2: 1\n", "thread 2: 2\n", "thread 2: 3\n",
    "thread 3: 1\n", "thread 3: 2\n", "thread 3: 3\n",
    "thread 4: 1\n", "thread 4: 2\n", "thread 4: 3\n"};

    size_t length = sizeof(strings) / sizeof(strings[0]);

    for (int i = 0; i < 4; i++) {
        args[i].lines = strings;
        args[i].threadCount = i;
        args[i].len = length;
        if (pthread_create(&threads[i], NULL, print_lines, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}