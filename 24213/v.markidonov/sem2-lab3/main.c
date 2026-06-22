#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define THREADS_NUM 4

void* print_strings(void *arg) {
    char **data = (char**)arg;

    while (*data) {
        printf("%s\n", *data);
        data++;
    }

    return NULL;
}

int main() {
    pthread_t threads[THREADS_NUM];
    char **data[THREADS_NUM];

    data[0] = (char*[]){"1.1", "1.2", "1.3", "1.4", "1.5", NULL};
    data[1] = (char*[]){"2.1", "2.2", "2.3", "2.4", NULL};
    data[2] = (char*[]){"3.1", "3.2", "3.3", NULL};
    data[3] = (char*[]){"4.1", "4.2", NULL};

    int code;
    for (int i = 0; i < THREADS_NUM; i++) {
        code = pthread_create(&threads[i], NULL, print_strings, data[i]);
        if (code != 0) {
            fprintf(stderr, "creating thread: %s\n", strerror(code));
            return 1;
        }
    }

    for (int i = 0; i < THREADS_NUM; i++) {
        code = pthread_join(threads[i], NULL);
        if (code != 0) {
            fprintf(stderr, "join thread: %s\n", strerror(code));
            return 1;
        }
    }

    return 0;
}
