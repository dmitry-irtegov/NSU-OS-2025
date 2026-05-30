#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void print_znak(void* arg) {
    int* val = (int*)arg;
    printf("znak %d\n", *val);
}


void* thread_function(void* arg) {
    int i = 1;
    pthread_cleanup_push(print_znak, &i);
    while (1) {
        i++;
        sleep(0.1);
        //printf("thm %d\n", i);
        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
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