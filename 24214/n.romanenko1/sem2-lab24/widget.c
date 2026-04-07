#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_WIDGETS 5
#define TIME_A 1
#define TIME_B 2
#define TIME_C 3

static sem_t sem_partA;
static sem_t sem_partB;
static sem_t sem_partC;
static sem_t sem_module;

static pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER;

static void say(const char *msg)
{
    pthread_mutex_lock(&print_mtx);
    puts(msg);
    fflush(stdout);
    pthread_mutex_unlock(&print_mtx);
}

static void *produce_A(void *arg)
{
    (void)arg;
    for (int i = 0; i < NUM_WIDGETS; ++i) {
        sleep(TIME_A);
        say("  [A] part A ready");
        sem_post(&sem_partA);
    }
    return NULL;
}

static void *produce_B(void *arg)
{
    (void)arg;
    for (int i = 0; i < NUM_WIDGETS; ++i) {
        sleep(TIME_B);
        say("  [B] part B ready");
        sem_post(&sem_partB);
    }
    return NULL;
}

static void *produce_C(void *arg)
{
    (void)arg;
    for (int i = 0; i < NUM_WIDGETS; ++i) {
        sleep(TIME_C);
        say("  [C] part C ready");
        sem_post(&sem_partC);
    }
    return NULL;
}

static void *assemble_module(void *arg)
{
    (void)arg;
    for (int i = 0; i < NUM_WIDGETS; ++i) {
        sem_wait(&sem_partA);
        sem_wait(&sem_partB);
        say("  [M] module assembled (A + B)");
        sem_post(&sem_module);
    }
    return NULL;
}

static void *assemble_widget(void *arg)
{
    (void)arg;
    for (int i = 0; i < NUM_WIDGETS; ++i) {
        sem_wait(&sem_module);
        sem_wait(&sem_partC);
        printf("[W] widget #%d assembled (module + C)\n", i + 1);
        fflush(stdout);
    }
    return NULL;
}

int main(void)
{
    sem_init(&sem_partA,  0, 0);
    sem_init(&sem_partB,  0, 0);
    sem_init(&sem_partC,  0, 0);
    sem_init(&sem_module, 0, 0);

    pthread_t tA, tB, tC, tM, tW;
    pthread_create(&tA, NULL, produce_A,       NULL);
    pthread_create(&tB, NULL, produce_B,       NULL);
    pthread_create(&tC, NULL, produce_C,       NULL);
    pthread_create(&tM, NULL, assemble_module, NULL);
    pthread_create(&tW, NULL, assemble_widget, NULL);

    pthread_join(tW, NULL);

    pthread_cancel(tA);
    pthread_cancel(tB);
    pthread_cancel(tC);
    pthread_cancel(tM);

    pthread_join(tA, NULL);
    pthread_join(tB, NULL);
    pthread_join(tC, NULL);
    pthread_join(tM, NULL);

    sem_destroy(&sem_partA);
    sem_destroy(&sem_partB);
    sem_destroy(&sem_partC);
    sem_destroy(&sem_module);

    printf("Production complete: %d widgets made.\n", NUM_WIDGETS);
    return 0;
}
