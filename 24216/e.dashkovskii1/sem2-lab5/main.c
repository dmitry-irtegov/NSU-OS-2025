#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void cleanup_handler(void* arg) {
    const char* cleanup_msg = "Дочерняя нить: выполнение обработчика очистки (завершаюсь)\n";
    write(STDOUT_FILENO, cleanup_msg, strlen(cleanup_msg));
}

void* child_routine(void* arg) {
    const char* child_msg = "Дочерняя нить: работаю...\n";
    
    pthread_cleanup_push(cleanup_handler, NULL);

    while (1) {
        sleep(1);
        write(STDOUT_FILENO, child_msg, strlen(child_msg)); 
    }

    pthread_cleanup_pop(0);
    
    return NULL;
}

int main() {
    pthread_t child_tid;
    void* result;
    int err;

    const char* msg_start = "Родитель: создаю дочернюю нить...\n";
    write(STDOUT_FILENO, msg_start, strlen(msg_start));

    err = pthread_create(&child_tid, NULL, child_routine, NULL);
    if (err != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    const char* msg_wait = "Родитель: жду 2 секунды...\n";
    write(STDOUT_FILENO, msg_wait, strlen(msg_wait));
    sleep(2);
    
    err = pthread_cancel(child_tid);
    if (err != 0) {
        perror("pthread_cancel");
        return EXIT_FAILURE;
    }

    err = pthread_join(child_tid, &result);
    if (err != 0) {
        perror("pthread_join");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}