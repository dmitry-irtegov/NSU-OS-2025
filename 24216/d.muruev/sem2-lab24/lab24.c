#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

void handleThread(int code, const char *msg) {
    if (code != 0) {
        fprintf(stderr, "%s %s\n", msg, strerror(code));
        exit(EXIT_FAILURE);
    }
}

void handleError(int code, const char *msg) {
    if (code != 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

sem_t sem_a;
sem_t sem_b;
sem_t sem_c;
sem_t sem_module;

void* produce_a(void* arg) {
    (void)arg;
    while (1) {
        sleep(1);
        handleError(sem_post(&sem_a), "error post a");
        fprintf(stderr, "Part A produced\n");
    }
    return NULL;
}

void* produce_b(void* arg) {
    (void)arg;
    while (1) {
        sleep(2);
        handleError(sem_post(&sem_b), "error post b");
        fprintf(stderr, "Part B produced\n");
    }
    return NULL;
}


void* produce_c(void* arg) {
    (void)arg;
    while (1) {
        sleep(3);
        handleError(sem_post(&sem_c), "error post c");
        fprintf(stderr, "Part C produced\n");
    }
    return NULL;
}


void* produce_module(void* arg) {
    (void)arg;
    while (1) {
        handleError(sem_wait(&sem_a), "error wait a");
        handleError(sem_wait(&sem_b), "error wait b");
        handleError(sem_post(&sem_module), "error post module");
        fprintf(stderr, "Module produced (from A and B)\n");
    }
    return NULL;
}

void* produce_widget(void* arg) {
    (void)arg;
    while (1) {
        handleError(sem_wait(&sem_c), "error wait c");
        handleError(sem_wait(&sem_module), "error wait module");
        fprintf(stderr, "Widget produced (from Mod. and C)\n");
    }
    return NULL;
}

int main() {

    handleError(sem_init(&sem_a, 0, 0), "error init a");
    handleError(sem_init(&sem_b, 0, 0), "error init b");
    handleError(sem_init(&sem_c, 0, 0), "error init c");
    handleError(sem_init(&sem_module, 0, 0), "error init module");


    pthread_t thread_a, thread_b, thread_c, thread_mod, thread_wid;

    handleThread(pthread_create(&thread_a, NULL, produce_a, NULL), "error create a");
    handleThread(pthread_create(&thread_b, NULL, produce_b, NULL), "error create b");
    handleThread(pthread_create(&thread_c, NULL, produce_c, NULL), "error create c");
    handleThread(pthread_create(&thread_mod, NULL, produce_module, NULL), "error create mod");
    handleThread(pthread_create(&thread_wid, NULL, produce_widget, NULL), "error create wid");


    handleThread(pthread_join(thread_wid, NULL), "error join wid");

    return 0;
}
