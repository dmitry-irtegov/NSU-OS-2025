#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define THREADS_NUM 4

void* worker_routine(void* arg) {
    const char** messages = (const char**)arg;

    for (int i = 0; messages[i] != NULL; i++) {
        write(STDOUT_FILENO, messages[i], strlen(messages[i]));
    }

    return NULL;
}

int main() {
    pthread_t tids[THREADS_NUM];
    int err;

    const char* list1[] = { "t1: golang\n", NULL };
    const char* list2[] = { "t2: java\n",  "t2: java\n", "t2: java\n", NULL };
    const char* list3[] = { "t3: C\n", NULL };
    const char* list4[] = { "t4:t4\n", NULL };

    const char** all_data[THREADS_NUM] = { list1, list2, list3, list4 };

    const char* msg_create = "Родитель: запускаю 4 рабочие нити...\n";
    write(STDOUT_FILENO, msg_create, strlen(msg_create));

    for (int i = 0; i < THREADS_NUM; i++) {
        err = pthread_create(&tids[i], NULL, worker_routine, (void*)all_data[i]);
        if (err != 0) {
            const char* err_msg = "Ошибка: не удалось создать нить\n";
            write(STDERR_FILENO, err_msg, strlen(err_msg));
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < THREADS_NUM; i++) {
        err = pthread_join(tids[i], NULL);
        if (err != 0) {
            const char* err_msg = "Ошибка: сбой при выполнении join\n";
            write(STDERR_FILENO, err_msg, strlen(err_msg));
            return EXIT_FAILURE;
        }
    }

    const char* msg_done = "Родитель: все нити завершили работу.\n";
    write(STDOUT_FILENO, msg_done, strlen(msg_done));

    return EXIT_SUCCESS;
}