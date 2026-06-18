#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* thread_function(void* arg) {
    for (int i = 1; i <= 10; i++) {
        printf("Дочерняя нить: строка %d\n", i);
        usleep(100000);
    }
    return NULL;
}

int main() {
    pthread_t thread_id;

    if (pthread_create(&thread_id, NULL, thread_function, NULL) != 0) {
        perror("Ошибка при создании нити");
        return 1;
    }

    if (pthread_join(thread_id, NULL) != 0) {
        perror("Ошибка при ожидании нити");
        return 1;
    }

    printf("\n--- Дочерняя нить завершилась, родитель продолжает работу ---\n\n");
    
    for (int i = 1; i <= 10; i++) {
        printf("Родительская нить: строка %d\n", i);
        usleep(100000);
    }
    
    return 0;
}

// почему при смерти родителя умирает остальные нити?     это очев...