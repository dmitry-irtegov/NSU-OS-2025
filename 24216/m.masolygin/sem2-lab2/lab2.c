#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void print_lines(char* str) {
    for (int i = 1; i <= 10; i++) {
        printf("%s %d\n", str, i);
    }
}

void* printer_ten(void* arg) {
    print_lines((char*)arg);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;

    if (pthread_create(&thread, NULL, printer_ten, "Child string:") != 0) {
        perror("Error creating thread");
        return 1;
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("Error joining thread");
        return 1;
    }

    print_lines("Parent string:");

    return 0;
}
