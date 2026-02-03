#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* ten_strings() {
    for (int i = 0; i < 9; i++) {
        printf("Ten strings.\n");
    }
    printf("Tenth string.\n");
    return NULL;
}

int main() {
    pthread_t thread;
    int code;

    code = pthread_create(&thread, NULL, ten_strings, NULL);
    if (code != 0) {
        fprintf(stderr, "Error creating thread.\n");
        return 1;
    }

    code = pthread_join(thread, NULL);
    if (code != 0) {
        fprintf(stderr, "Error joining thread.\n");
        return 1;
    }
    for (int i = 0; i < 9; i++) {
        printf("Ten strings.\n");
    }
    printf("Tenth string.\n");

    return 0;
}