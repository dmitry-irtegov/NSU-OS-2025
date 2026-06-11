#include <stdio.h>
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void *print_child_lines(void *arg) {
    for (int i = 0; i < 10; i++) {
        printf("Дочерняя нить: %d\n", i + 1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    if (pthread_create(&thread, NULL, print_child_lines, NULL) != 0) {
        perror("Ошибка при создании нити");
        return 1;
    }

    // Родительская нить выводит свои строки
    for (int i = 0; i < 10; i++) {
      printf("Родительская нить: %d\n", i + 1);
    }
    if (pthread_join(thread, NULL) != 0) {
        perror("Ошибка при ожидании завершения нити");
        return 1;
    }

    return 0;
}
