#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *thread_print(void *data) {
    char **strings = (char **)data;
    for (int i = 0; strings[i] != NULL; i++) {
        fprintf(stderr, "%s\n", strings[i]);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[4];
    int err;
    char *seq1[] = {"Thread 1: Hello", "Thread 1: from", "Thread 1: the first", "Thread 1: thread!", NULL};
    char *seq2[] = {"Thread 2: Printing", "Thread 2: another", "Thread 2: sequence.", NULL};
    char *seq3[] = {"Thread 3: Just", "Thread 3: two lines.", NULL};
    char *seq4[] = {"Thread 4: One", "Thread 4: Two", "Thread 4: Three", "Thread 4: Four", "Thread 4: Five!", NULL};
    char **sequences[4] = {seq1, seq2, seq3, seq4};
    for (int i = 0; i < 4; i++) {
        err = pthread_create(&threads[i], NULL, thread_print, sequences[i]);
        if (err != 0) {
            fprintf(stderr, "error create thread %d: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < 4; i++) {
        err = pthread_join(threads[i], NULL);
        if (err != 0) {
            fprintf(stderr, "error join thread %d: %s\n", i, strerror(err));
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}