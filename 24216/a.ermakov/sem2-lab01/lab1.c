#include <stdio.h>
#include <pthread.h>

void print_lines(const char* thread_name) {
    for (int i = 1; i <= 10; i++) {
        printf("%s: строка %d\n", thread_name, i);
    }
}

void* thread_routine(void* arg) {
    print_lines("Дочерняя нить");
    return NULL;
}

int main() {
    pthread_t thr;

    if (pthread_create(&thr, NULL, thread_routine, NULL) != 0) {
        perror("Error creating thread");
        return 1;
    }

    print_lines("Родительская нить");

    pthread_join(thr, NULL);
    return 0;
}
