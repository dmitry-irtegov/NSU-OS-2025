#include <stdio.h>
#include <pthread.h>
#include <string.h>

void print_lines(const char* thread_name) {
    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "%s: строка %d\n", thread_name, i);
    }
}

void* thread_routine(void* arg) {
    (void) arg;
    print_lines("Дочерняя нить");
    return NULL;
}

int main() {
    pthread_t thr;
    int status;
    status = pthread_create(&thr, NULL, thread_routine, NULL);
    if  (status != 0) {
        fprintf(stderr, "Ошибка создания потока: %s\n", strerror(status));
        return 1;
    }
    status = pthread_join(thr, NULL);
    if(status != 0) {
        fprintf(stderr, "Ошибка при ожидании потока (join): %s\n", strerror(status));
        return 1;
    }
    print_lines("Родительская нить");
    
    return 0;
}
