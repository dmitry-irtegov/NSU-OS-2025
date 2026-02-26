#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

void print_text(char* text) {
    for(int i = 0; i < 10; i++) {
        fprintf(stderr, "%s\n", text);
    }
}

void* thread_work(void* param) {
    print_text("Im child");
    return NULL;
}

int main() {
    pthread_t thread;
    int status;

    status = pthread_create(&thread, NULL, thread_work, NULL);
    if (status != 0) {
        fprintf(stderr, "Ошибка создания потока %d: %s\n", status, strerror(status));
        return EXIT_FAILURE;
    }

    print_text("Im parent");
    status = pthread_join(thread, NULL);
    if (status != 0) {
        fprintf(stderr, "Ошибка ожидания потока: %s\n", strerror(status));
    }

    return EXIT_SUCCESS;
}
