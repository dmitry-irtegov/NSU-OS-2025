#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define str "Child string\n"

void* print_line(void* arg) {
    (void)arg;
    while (1) {
        write(STDOUT_FILENO, str, strlen(str));
        sleep(1);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int err = pthread_create(&thread, NULL, print_line, NULL);
    if (err != 0) {
        fprintf(stderr, "err creating threads: %s\n", strerr(err));
        exit(EXIT_FAILURE);
    }
    sleep(2);

    pthread_cancel(thread);
    err = pthread_join(thread, NULL);
    if (err != 0) {
        fprintf(stderr, "err joining thread: %s\n", strerr(err));
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}