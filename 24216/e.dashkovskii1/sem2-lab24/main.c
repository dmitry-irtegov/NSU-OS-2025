#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define WIDGETS 5

sem_t parts_a;
sem_t parts_b;
sem_t parts_c;
sem_t modules;
sem_t widgets_done;

static void *make_a(void *arg) {
    (void)arg;
    for (;;) {
        sleep(1);
        printf("part A ready\n");
        fflush(stdout);
        sem_post(&parts_a);
    }
    return NULL;
}

static void *make_b(void *arg) {
    (void)arg;
    for (;;) {
        sleep(2);
        printf("part B ready\n");
        fflush(stdout);
        sem_post(&parts_b);
    }
    return NULL;
}

static void *make_c(void *arg) {
    (void)arg;
    for (;;) {
        sleep(3);
        printf("part C ready\n");
        fflush(stdout);
        sem_post(&parts_c);
    }
    return NULL;
}

static void *make_module(void *arg) {
    (void)arg;
    for (;;) {
        sem_wait(&parts_a);
        sem_wait(&parts_b);
        printf("module assembled (A + B)\n");
        fflush(stdout);
        sem_post(&modules);
    }
    return NULL;
}

static void *make_widget(void *arg) {
    int i;

    (void)arg;
    for (i = 0; i < WIDGETS; i++) {
        sem_wait(&modules);
        sem_wait(&parts_c);
        printf("widget %d assembled (module + C)\n", i + 1);
        fflush(stdout);
    }
    sem_post(&widgets_done);
    return NULL;
}

int main(void) {
    pthread_t ta, tb, tc, tm, tw;

    sem_init(&parts_a, 0, 0);
    sem_init(&parts_b, 0, 0);
    sem_init(&parts_c, 0, 0);
    sem_init(&modules, 0, 0);
    sem_init(&widgets_done, 0, 0);

    pthread_create(&ta, NULL, make_a, NULL);
    pthread_create(&tb, NULL, make_b, NULL);
    pthread_create(&tc, NULL, make_c, NULL);
    pthread_create(&tm, NULL, make_module, NULL);
    pthread_create(&tw, NULL, make_widget, NULL);

    sem_wait(&widgets_done);
    printf("%d widgets produced, line stops\n", WIDGETS);

    pthread_cancel(ta);
    pthread_cancel(tb);
    pthread_cancel(tc);
    pthread_cancel(tm);

    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    pthread_join(tc, NULL);
    pthread_join(tm, NULL);
    pthread_join(tw, NULL);

    sem_destroy(&parts_a);
    sem_destroy(&parts_b);
    sem_destroy(&parts_c);
    sem_destroy(&modules);
    sem_destroy(&widgets_done);
    return 0;
}
