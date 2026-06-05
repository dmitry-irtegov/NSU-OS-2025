#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
pthread_mutex_t foodlock;

void *philosopher(void *id);
int food_on_table(void);
void get_fork(int phil, int fork, const char *hand);
void down_forks(int f1, int f2);

int main(void) {
    int i;

    pthread_mutex_init(&foodlock, NULL);
    for (i = 0; i < PHILO; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }

    for (i = 0; i < PHILO; i++) {
        pthread_create(&phils[i], NULL, philosopher, (void *)(long)i);
    }
    for (i = 0; i < PHILO; i++) {
        pthread_join(phils[i], NULL);
    }

    for (i = 0; i < PHILO; i++) {
        pthread_mutex_destroy(&forks[i]);
    }
    pthread_mutex_destroy(&foodlock);
    return 0;
}

void *philosopher(void *num) {
    int id = (int)(long)num;
    int left_fork, right_fork, first_fork, second_fork, f;

    printf("Philosopher %d sitting down to dinner.\n", id);
    right_fork = id;
    left_fork = id + 1;
    if (left_fork == PHILO) {
        left_fork = 0;
    }

    if (left_fork < right_fork) {
        first_fork = left_fork;
        second_fork = right_fork;
    } else {
        first_fork = right_fork;
        second_fork = left_fork;
    }

    while ((f = food_on_table())) {
        printf("Philosopher %d: get dish %d.\n", id, f);
        get_fork(id, first_fork, "first ");
        get_fork(id, second_fork, "second");

        printf("Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));
        down_forks(first_fork, second_fork);
    }

    printf("Philosopher %d is done eating.\n", id);
    return NULL;
}

int food_on_table(void) {
    static int food = FOOD;
    int myfood;

    pthread_mutex_lock(&foodlock);
    if (food > 0) {
        food--;
    }
    myfood = food;
    pthread_mutex_unlock(&foodlock);
    return myfood;
}

void get_fork(int phil, int fork, const char *hand) {
    pthread_mutex_lock(&forks[fork]);
    printf("Philosopher %d: got %s fork %d\n", phil, hand, fork);
}

void down_forks(int f1, int f2) {
    pthread_mutex_unlock(&forks[f1]);
    pthread_mutex_unlock(&forks[f2]);
}
