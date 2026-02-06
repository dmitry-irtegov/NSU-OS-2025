#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_lines(char* str) {
    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "%s %d\n", str, i);
    }
}

void* printer_ten(void* arg) {
    print_lines((char*)arg);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;

    int error = pthread_create(&thread, NULL, printer_ten, "Child string:");

    if (error != 0) {
        fprintf(stderr, "Error creating threads: %s\n", strerror(error));
        return 1;
    }

    error = pthread_join(thread, NULL);

    if (error != 0) {
        fprintf(stderr, "Error joining threads: %s\n", strerror(error));
        return 1;
    }

    print_lines("Parent string:");

    return 0;
}
