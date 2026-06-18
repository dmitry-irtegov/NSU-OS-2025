#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


void* thread_function(void* arg) {
    int i = 1;
    while (1) {
        i++;
        pthread_testcancel();
    }
    return NULL;
}

int main() {
    pthread_t thread_id;

    if (pthread_create(&thread_id, NULL, thread_function, NULL) != 0) {
        perror("Ошибка при создании нити");
        return 1;
    }

    sleep(2);

    printf("Clancel\n");
    if (pthread_cancel(thread_id) != 0) {
        perror("Ошибка при отмене нити");
    }

    if (pthread_join(thread_id, NULL) != 0) {
        perror("Ошибка при ожидании нити");
        return 1;
    }

    printf("all\n");
    return 0;
}