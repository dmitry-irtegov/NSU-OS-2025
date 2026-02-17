/*
 * File:   din_phil.c
 * Author: nd159473 (Nickolay Dalmatov, Sun Microsystems)
 * adapted from
 * http://developers.sun.com/sunstudio/downloads/ssx/tha/tha_using_deadlock.html
 *
 * Created on January 1, 1970, 9:53 AM
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
void *philosopher(void *id);
int food_on_table();
void get_fork(int, int, char *);
void down_forks(int, int);
pthread_mutex_t foodlock;

int sleep_seconds = 0;

int main(int argc, char **argv) {
    if (argc == 2)
        sleep_seconds = atoi(argv[1]);

    pthread_mutex_init(&foodlock, NULL);
    for (int i = 0; i < PHILO; i++)
        pthread_mutex_init(&forks[i], NULL);
    for (int i = 0; i < PHILO; i++)
        pthread_create(&phils[i], NULL, philosopher, (void *)i);
    for (int i = 0; i < PHILO; i++)
        pthread_join(phils[i], NULL);
    return 0;
}

void *philosopher(void *num) {
    int second_fork, first_fork, food;
    int id = (int)num;
    printf("Philosopher %d sitting down to dine.\n", id);

    if (id != PHILO - 1) {
        first_fork = id;
        second_fork = id + 1;
    } else {
        first_fork = 0;
        second_fork = id;
    }

    while ((food = food_on_table())) {

        /* Thanks to philosophers #1 who would like to
         * take a nap before picking up the forks, the other
         * philosophers may be able to eat their dishes and
         * not deadlock.
         */
        if (id == 1)
            sleep(sleep_seconds);

        printf("Philosopher %d: get dish %d.\n", id, food);
        get_fork(id, first_fork, "first");
        get_fork(id, second_fork, "second ");

        printf("Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - food + 1));
        down_forks(first_fork, second_fork);
    }
    printf("Philosopher %d is done eating.\n", id);
    return NULL;
}

int food_on_table() {
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

void get_fork(int phil, int fork, char *hand) {
    pthread_mutex_lock(&forks[fork]);
    printf("Philosopher %d: got %s fork %d\n", phil, hand, fork);
}

void down_forks(int f1, int f2) {
    pthread_mutex_unlock(&forks[f1]);
    pthread_mutex_unlock(&forks[f2]);
}
