#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* sequences[4][6] = {
    {"A1", "A2", "A3", "A4", "A5", "A6"},
    {"B1", "B2", "B3", "B4", "B5", "B6"},
    {"C1", "C2", "C3", "C4", "C5", "C6"},
    {"D1", "D2", "D3", "D4", "D5", "D6"}
};

struct thread_args {
    const char** seq;
    int lines;
};

void* thread_function(void* arg) {
    struct thread_args *a = (struct thread_args*)arg;
    
    for (int i = 0; i < a->lines; i++) {
        printf("%s\n", a->seq[i]);
    }
    
    return NULL;
}

int main() {
    pthread_t threads[4];
    struct thread_args args[4] = {
        {sequences[0], 3},
        {sequences[1], 4},
        {sequences[2], 5},
        {sequences[3], 2}
    };
    int err;

    for (int i = 0; i < 4; i++) {
        err = pthread_create(&threads[i], NULL, thread_function, &args[i]);
        if (err != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            return 1;
        }
    }
    
    for (int i = 0; i < 4; i++) {
        err = pthread_join(threads[i], NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(err));
            return 1;
        }
    }
    
    printf("All threads completed\n");
    return 0;
}
