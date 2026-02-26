#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t semA, semB, semC, semModule;

void handleThread(int code, const char *msg) {
    if (code != 0) {
        fprintf(stderr, "%s: %s\n", msg, strerror(code));
        exit(EXIT_FAILURE);
    }
}

void handleError(int code, const char *msg) {
    if (code != 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void* makeA(void *unused) {
    (void)unused;
    while (1) {
        sleep(1);
        handleError(sem_post(&semA), "sem_post A");
        fprintf(stderr, "produced A\n");
    }
}

void* makeB(void *unused) {
    (void)unused;
    while (1) {
        sleep(2);
        handleError(sem_post(&semB), "sem_post B");
        fprintf(stderr, "produced B\n");
    }
}

void* makeC(void *unused) {
    (void)unused;
    while (1) {
        sleep(3);
        handleError(sem_post(&semC), "sem_post C");
        fprintf(stderr, "produced C\n");
    }
}

void* createModule(void *unused) {
    (void)unused;
    while (1) {
        handleError(sem_wait(&semA), "sem_wait A");
        handleError(sem_wait(&semB), "sem_wait B");
        fprintf(stderr, "create module with A and B\n");
        handleError(sem_post(&semModule), "sem_post Module");
    }
}

void* createBolt(void *unused) {
    (void)unused;
    int count = 0;
    while (1) {
        handleError(sem_wait(&semModule), "sem_wait Module");
        handleError(sem_wait(&semC), "sem_wait C");
        count++;
        fprintf(stderr, "create bolt number %d\n", count);
    }
}

int main() {
    handleError(sem_init(&semA, 0, 0), "sem_init A");
    handleError(sem_init(&semB, 0, 0), "sem_init B");
    handleError(sem_init(&semC, 0, 0), "sem_init C");
    handleError(sem_init(&semModule, 0, 0), "sem_init Module");

    pthread_t a_thread, b_thread, c_thread, module_thread, widget_thread;

    handleThread(pthread_create(&a_thread, NULL, makeA, NULL), "pthread_create A");
    handleThread(pthread_create(&b_thread, NULL, makeB, NULL), "pthread_create B");
    handleThread(pthread_create(&c_thread, NULL, makeC, NULL), "pthread_create C");
    handleThread(pthread_create(&module_thread, NULL, createModule, NULL), "pthread_create Module");
    handleThread(pthread_create(&widget_thread, NULL, createBolt, NULL), "pthread_create Bolt");

    handleThread(pthread_join(a_thread, NULL), "pthread_join A");
    handleThread(pthread_join(b_thread, NULL), "pthread_join B");
    handleThread(pthread_join(c_thread, NULL), "pthread_join C");
    handleThread(pthread_join(module_thread, NULL), "pthread_join Module");
    handleThread(pthread_join(widget_thread, NULL), "pthread_join Bolt");

    return 0;
}
