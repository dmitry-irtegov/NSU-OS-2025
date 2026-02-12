#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define str "Child string\n"
#define str_clean "Called clean-up handler\n"

void cleanup_handler(void* arg) {
    (void)arg;
    write(STDOUT_FILENO, str_clean, strlen(str_clean));
}

void* print_line(void* arg) {
    (void)arg;
    pthread_cleanup_push(cleanup_handler, NULL);
    while (1) {
        write(STDOUT_FILENO, str, strlen(str));
        sleep(1);
    }

    pthread_cleanup_pop(0);
    return NULL;
}

int main() {
    pthread_t thread;

    int error = pthread_create(&thread, NULL, print_line, NULL);

    if (error != 0) {
        fprintf(stderr, "Error creating threads: %s\n", strerror(error));
        return 1;
    }

    sleep(2);

    pthread_cancel(thread);

    error = pthread_join(thread, NULL);
    if (error != 0) {
        fprintf(stderr, "Error joining thread: %s\n", strerror(error));
        return 1;
    }

    return 0;
}
