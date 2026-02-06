#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define handle_error_en(en, msg) do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void print_all_lines(const char* string) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "%s\n", string);
    }
}

void *print_lines(void *arg) {
    (void)arg;
    print_all_lines("thread");
    return NULL;
}

int main() {
    pthread_t tid;
    int result;

    result = pthread_create(&tid, NULL, &print_lines, NULL);
    if (result != 0) {
        handle_error_en(result, "pthread_create");
    }

    print_all_lines("parent");

    result = pthread_join(tid, NULL);
    if (result != 0) {
        handle_error_en(result, "pthread_join");
    }

    exit(EXIT_SUCCESS);
}
