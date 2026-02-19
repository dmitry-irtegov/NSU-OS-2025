#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void* child_routine(void* arg) {
    const char* child_msg = "Дочерняя нить: работаю...\n";
    while (1) {
        sleep(1);
        write(STDOUT_FILENO, child_msg, strlen(child_msg)); 
    }
    
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
        const char* err_msg = "Ошибка: не удалось создать нить\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return EXIT_FAILURE;
    }

    const char* msg_wait = "Родитель: жду 2 секунды...\n";
    write(STDOUT_FILENO, msg_wait, strlen(msg_wait));
    sleep(2);
    
    err = pthread_cancel(child_tid);
    if (err != 0) {
        const char* err_msg = "Ошибка: не удалось pthread_cancel\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return EXIT_FAILURE;
    }

    err = pthread_join(child_tid, &result);
    if (err != 0) {
        const char* err_msg = "Ошибка: сбой при pthread_join\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}