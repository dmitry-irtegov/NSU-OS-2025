/* 
 * File:   din_phil.c
 * Author: nd159473 (Nickolay Dalmatov, Sun Microsystems)
 * adapted from http://developers.sun.com/sunstudio/downloads/ssx/tha/tha_using_deadlock.html
 *
 * Created on January 1, 1970, 9:53 AM
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t fork_mas[PHILO];
pthread_mutex_t forks;
pthread_cond_t forks_cond;
pthread_t phils[PHILO];

void *philosopher (void *id);
int food_on_table ();
void get_fork (int, int, int);
void down_forks (int, int);
pthread_mutex_t foodlock;

int sleep_seconds = 0;

int main (int argn, char **argv) {
    int i;
    if (argn == 2) sleep_seconds = atoi (argv[1]);

    pthread_mutex_init (&foodlock, NULL);
    pthread_mutex_init (&forks, NULL);
    pthread_cond_init (&forks_cond, NULL);
    for (i = 0; i < PHILO; i++)
        pthread_mutex_init (&fork_mas[i], NULL);
        
    for (i = 0; i < PHILO; i++)
        pthread_create (&phils[i], NULL, philosopher, (void *)i);
        
    for (i = 0; i < PHILO; i++)
        pthread_join (phils[i], NULL);
    pthread_mutex_destroy(&forks);
    pthread_cond_destroy(&forks_cond);
    for (i = 0; i < PHILO; i++)
        pthread_mutex_destroy(&fork_mas[i]);
        
    return 0;
}

void *philosopher (void *num) {
    int id;
    int left_fork, right_fork, f;

    id = (int)num;
    printf ("Philosopher %d sitting down to dinner.\n", id);
    right_fork = id;
    left_fork = id + 1;
 
    /* Wrap around the forks. */
    if (left_fork == PHILO)
        left_fork = 0;
 
    while (f = food_on_table ()) {
        printf ("Philosopher %d: get dish %d.\n", id, f);
        get_fork (id, left_fork, right_fork);
        
        printf ("Philosopher %d: eating.\n", id);
        usleep (DELAY * (FOOD - f + 1));
        down_forks (left_fork, right_fork);
    }
    printf ("Philosopher %d is done eating.\n", id);
    return (NULL);
}

int food_on_table () {
    static int food = FOOD;
    int myfood;

    pthread_mutex_lock (&foodlock);
    if (food > 0) {
        food--;
    }
    myfood = food;
    pthread_mutex_unlock (&foodlock);
    return myfood;
}

void get_fork (int phil, int left, int right) {
    pthread_mutex_lock(&forks);
    while (1) {
        int got_left = (pthread_mutex_trylock(&fork_mas[left]) == 0);
        int got_right = (pthread_mutex_trylock(&fork_mas[right]) == 0);
        if (got_left && got_right) {
            break;
        }
        if (got_left) pthread_mutex_unlock(&fork_mas[left]);
        if (got_right) pthread_mutex_unlock(&fork_mas[right]);
        pthread_cond_wait(&forks_cond, &forks);
    }
    pthread_mutex_unlock(&forks);
    printf ("Philosopher %d: got left fork %d and right fork %d\n", phil, left, right);
}

void down_forks (int f1, int f2) {
    pthread_mutex_lock(&forks);
    pthread_mutex_unlock(&fork_mas[f1]);
    pthread_mutex_unlock(&fork_mas[f2]);
    pthread_cond_broadcast(&forks_cond);    
    pthread_mutex_unlock(&forks);
}
