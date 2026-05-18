/* 
 * File:   din_phil.c
 * Author: nd159473 (Nickolay Dalmatov, Sun Microsystems)
 * adapted from http://developers.sun.com/sunstudio/downloads/ssx/tha/tha_using_deadlock.html
 *
 * Created on January 1, 1970, 9:53 AM
 */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

enum { THINKING, HUNGRY, EATING, FINISHED } state[PHILO];

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
pthread_mutex_t foodlock;
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv[PHILO];

int food_count = FOOD;

void *philosopher(void *num);
int get_food();
int check_food();
void test(int id);

int main(int argc, char **argv) {
    int i;
    (void)argc; 
    (void)argv;

    pthread_mutex_init(&foodlock, NULL);
    for (i = 0; i < PHILO; i++) {
        pthread_mutex_init(&forks[i], NULL);
        pthread_cond_init(&cv[i], NULL);
        state[i] = THINKING;
    }

    for (i = 0; i < PHILO; i++) {
        pthread_create(&phils[i], NULL, philosopher, (void *)(long)i);
    }

    for (i = 0; i < PHILO; i++) {
        pthread_join(phils[i], NULL);
    }

    pthread_mutex_destroy(&foodlock);
    pthread_mutex_destroy(&global_lock);
    for (i = 0; i < PHILO; i++) {
        pthread_mutex_destroy(&forks[i]);
        pthread_cond_destroy(&cv[i]);
    }

    printf("Dinner is over. All threads have completed successfully.\n");
    return 0;
}

void test(int id) {
    int left_neighbor = (id + PHILO - 1) % PHILO;
    int right_neighbor = (id + 1) % PHILO;

    if (state[id] == HUNGRY && state[left_neighbor] != EATING && state[right_neighbor] != EATING) {
        if (pthread_mutex_trylock(&forks[id]) == 0) {
            printf ("Philosopher %d:  got left fork. %d\n", id, id);

            if (pthread_mutex_trylock(&forks[(id + 1) % PHILO]) == 0) {
                printf ("Philosopher %d: got right fork. %d\n", id, (id + 1) % PHILO);
                state[id] = EATING;
                pthread_cond_signal(&cv[id]); 
                return;
            }
            printf ("Philosopher %d: couldn't get right fork. %d\n Put left fork down. %d\n", id, (id + 1) % PHILO, id);
            pthread_mutex_unlock(&forks[id]);
        }
    }
    
    if (check_food() <= 0 && state[id] != EATING) {
        pthread_cond_signal(&cv[id]);
    }
}

void *philosopher(void *num) {
    int id = (int)(long)num;
    int f;
    printf ("Philosopher %d sitting down to dinner.\n", id);

    while (1) {
        if (check_food() <= 0) break;

        pthread_mutex_lock(&global_lock);
        state[id] = HUNGRY;
        test(id);

        while (state[id] == HUNGRY) {
            if (check_food() <= 0) {
                state[id] = FINISHED;
                break;
            }
            pthread_cond_wait(&cv[id], &global_lock);
        }

        if (state[id] == FINISHED) {
            pthread_mutex_unlock(&global_lock);
            break;
        }
        pthread_mutex_unlock(&global_lock);

        f = get_food();
        if (f <= 0) {
            pthread_mutex_lock(&global_lock);
            state[id] = FINISHED;
            pthread_mutex_unlock(&forks[id]);
            pthread_mutex_unlock(&forks[(id + 1) % PHILO]);
            pthread_mutex_unlock(&global_lock);
            break;
        }

        printf ("Philosopher %d: get dish %d.\n", id, f);
        usleep(DELAY);

        pthread_mutex_lock(&global_lock);
        state[id] = THINKING;
        pthread_mutex_unlock(&forks[id]);
        printf ("Philosopher %d: put left fork down. %d\n", id, id);
        pthread_mutex_unlock(&forks[(id + 1) % PHILO]);
        printf ("Philosopher %d: put right fork down. %d\n", id, (id + 1) % PHILO);

        test((id + PHILO - 1) % PHILO); 
        test((id + 1) % PHILO);          
        pthread_mutex_unlock(&global_lock);
        
        usleep(DELAY / 10); 
    }

    pthread_mutex_lock(&global_lock);
    state[id] = FINISHED;
    test((id + PHILO - 1) % PHILO);
    test((id + 1) % PHILO);
    pthread_mutex_unlock(&global_lock);

    printf("Philosopher %d left the table.\n", id);
    return NULL;
}

int get_food() {
    int val;
    pthread_mutex_lock(&foodlock);
    if (food_count > 0) {
        val = food_count--;
    } else {
        val = 0;
    }
    pthread_mutex_unlock(&foodlock);
    return val;
}

int check_food() {
    int val;
    pthread_mutex_lock(&foodlock);
    val = food_count;
    pthread_mutex_unlock(&foodlock);
    return val;
}