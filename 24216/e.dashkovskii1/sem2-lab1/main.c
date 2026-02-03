#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define handle_error_en(en, msg) do {errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)

void print_all_lines(char* string) {
    for (int i = 0; i < 10; i++) {
        printf("%s\n", string);
    }
}

void *print_lines(void *arg) {
    (void)arg;
    print_all_lines("thread");
    pthread_exit(NULL);
}

int main() {
    pthread_t tid;
    int result;

    result = pthread_create(&tid, NULL, &print_lines, NULL);

    if (result != 0) {
        handle_error_en(result, "pthread_create");
    }

    print_all_lines("parent");
    pthread_join(tid, NULL);

    return 0;
}