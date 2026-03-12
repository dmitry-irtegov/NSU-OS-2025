#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <semaphore.h>

sem_t sem_parent;
sem_t sem_child;

void handle_error(int en, const char* msg) {
    if (en != 0) {
        errno = en;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void handle_sem_error(int res, const char* msg) {
    if (res == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void *print_lines(void *arg) {
    (void)arg;

    for (int i = 0; i < 10; i++) {
        handle_sem_error(sem_wait(&sem_child), "child: sem_wait");

        fprintf(stderr, "thread\n");

        handle_sem_error(sem_post(&sem_parent), "child: sem_post");
    }

    return NULL;
}

int main() {
    pthread_t tid;

    handle_sem_error(sem_init(&sem_parent, 0, 1), "init sem_parent");

    handle_sem_error(sem_init(&sem_child, 0, 0), "init sem_child");

    int result = pthread_create(&tid, NULL, &print_lines, NULL);
    handle_error(result, "pthread_create");

    for (int i = 0; i < 10; i++) {
        handle_sem_error(sem_wait(&sem_parent), "main: sem_wait");

        fprintf(stderr, "parent\n");

        handle_sem_error(sem_post(&sem_child), "main: sem_post");
    }

    result = pthread_join(tid, NULL);
    handle_error(result, "pthread_join");

    handle_sem_error(sem_destroy(&sem_parent), "destroy sem_parent");
    handle_sem_error(sem_destroy(&sem_child), "destroy sem_child");

    exit(EXIT_SUCCESS);
}