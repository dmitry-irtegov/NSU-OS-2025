#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void print_line(char* line) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "%s %d\n", line, i);
    }
}
void* thread_work(void* arg) {
    print_line((char*)arg);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread1;

    if (pthread_create(&thread1, NULL, thread_work, "Thread 1") != 0) {
        perror("Failed to create thread");
        return EXIT_FAILURE;
    }

    if (pthread_join(thread1, NULL) != 0) {
        perror("Failed to join thread");
        return EXIT_FAILURE;
    }

    print_line("Main thread");

    return EXIT_SUCCESS;
}