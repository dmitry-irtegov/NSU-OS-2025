#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* print_line(void* arg) {
    while (1) {
        fprintf(stderr, "Child!\n");
        sleep(1);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t thread;

    if (pthread_create(&thread, NULL, print_line, "Thread 1") != 0) {
        perror("Failed to create thread");
        return EXIT_FAILURE;
    }

    sleep(2);

    pthread_cancel(thread);

    if (pthread_join(thread, NULL) != 0) {
        perror("Failed to join thread");
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}