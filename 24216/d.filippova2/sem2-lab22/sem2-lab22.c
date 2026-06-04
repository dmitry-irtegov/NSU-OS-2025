#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];      
pthread_mutex_t forks_lock;           
pthread_cond_t forks_cond;            

pthread_mutex_t foodlock;
pthread_t phils[PHILO];

void *philosopher(void *id);
int food_on_table(void);
void get_forks(int phil, int left_fork, int right_fork);
void down_forks(int left_fork, int right_fork);

int main(int argn, char **argv){
    int i;

    (void)argn;
    (void)argv;

    pthread_mutex_init(&foodlock, NULL);
    pthread_mutex_init(&forks_lock, NULL);
    pthread_cond_init(&forks_cond, NULL);

    for (i = 0; i < PHILO; i++)
        pthread_mutex_init(&forks[i], NULL);

    for (i = 0; i < PHILO; i++)
        pthread_create(&phils[i], NULL, philosopher, (void *)(intptr_t)i);

    for (i = 0; i < PHILO; i++)
        pthread_join(phils[i], NULL);

    for (i = 0; i < PHILO; i++)
        pthread_mutex_destroy(&forks[i]);

    pthread_cond_destroy(&forks_cond);
    pthread_mutex_destroy(&forks_lock);
    pthread_mutex_destroy(&foodlock);

    return 0;
}

void *philosopher(void *num){
    int id;
    int left_fork, right_fork, f;

    id = (int)(intptr_t)num;
    printf("Philosopher %d sitting down to dinner.\n", id);

    right_fork = id;
    left_fork = id + 1;

    if (left_fork == PHILO)
        left_fork = 0;

    while ((f = food_on_table())) {
        printf("Philosopher %d: get dish %d.\n", id, f);

        get_forks(id, left_fork, right_fork);

        printf("Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));

        down_forks(left_fork, right_fork);
    }

    printf("Philosopher %d is done eating.\n", id);
    return NULL;
}

int food_on_table(void){
    static int food = FOOD;
    int myfood;

    pthread_mutex_lock(&foodlock);
    if (food > 0) {
        myfood = food;
        food--;
    } else {
        myfood = 0;
    }
    pthread_mutex_unlock(&foodlock);

    return myfood;
}

void get_forks(int phil, int left_fork, int right_fork){
    int got_left;
    int got_right;

    pthread_mutex_lock(&forks_lock);

    for (;;) {
        got_right = (pthread_mutex_trylock(&forks[right_fork]) == 0);
        got_left = (pthread_mutex_trylock(&forks[left_fork]) == 0);

        if (got_right && got_left) {
            printf("Philosopher %d: got right fork %d\n", phil, right_fork);
            printf("Philosopher %d: got left  fork %d\n", phil, left_fork);
            pthread_mutex_unlock(&forks_lock);
            return;
        }

        if (got_right)
            pthread_mutex_unlock(&forks[right_fork]);
        if (got_left)
            pthread_mutex_unlock(&forks[left_fork]);

        pthread_cond_wait(&forks_cond, &forks_lock);
    }
}

void down_forks(int left_fork, int right_fork){
    pthread_mutex_lock(&forks_lock);

    pthread_mutex_unlock(&forks[left_fork]);
    pthread_mutex_unlock(&forks[right_fork]);
    printf("Put down forks %d and %d\n", left_fork, right_fork);

    pthread_cond_broadcast(&forks_cond);
    pthread_mutex_unlock(&forks_lock);
}
