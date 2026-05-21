#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
void* philosopher(void* id);
int food_on_table();
void get_fork(int, int);
void down_forks(int, int);
pthread_mutex_t foodlock;
pthread_mutex_t fork_m;
pthread_cond_t condition;

int sleep_seconds = 0;

int
main(int argn,
    char** argv)
{
    int i;

    if (argn == 2)
        sleep_seconds = atoi(argv[1]);
    int ind[PHILO];

    pthread_mutex_init(&foodlock, NULL);
    pthread_mutex_init(&fork_m, NULL);
    pthread_cond_init(&condition, NULL);

    for (i = 0; i < PHILO; i++)
        pthread_mutex_init(&forks[i], NULL);
    for (i = 0; i < PHILO; i++) {
        ind[i] = i;
        pthread_create(&phils[i], NULL, philosopher, (void*)&ind[i]);
    }
    for (i = 0; i < PHILO; i++)
        pthread_join(phils[i], NULL);
    return 0;
}

void*
philosopher(void* num)
{
    int id;
    int left_fork, right_fork, f;

    id = *(int*)num;
    printf("Philosopher %d sitting down to dinner.\n", id);
    right_fork = id;
    left_fork = id + 1;

    if (left_fork == PHILO) {
        left_fork = 0;
    }

    while (f = food_on_table()) {

        if (id == 1) {
            sleep(sleep_seconds);
        }

        printf("Philosopher %d: get dish %d\n", id, f);

        pthread_mutex_lock(&fork_m);
        
        while (1) {
            if (pthread_mutex_trylock(&forks[right_fork]) == 0) {
                if (pthread_mutex_trylock(&forks[left_fork]) == 0) {
                    break;
                }
                pthread_mutex_unlock(&forks[right_fork]);
            }
            pthread_cond_wait(&condition, &fork_m);
        }
        pthread_mutex_unlock(&fork_m);

        printf("Philosopher %d: got %d and %d forks\n", id, right_fork, left_fork);

        printf("Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));
        
        down_forks(right_fork, left_fork);
    }
    printf("Philosopher %d is done eating.\n", id);
    return (NULL);
}
int food_on_table()
{
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


void down_forks(int first, int second) {
    pthread_mutex_lock(&fork_m);

    pthread_mutex_unlock(&forks[second]);
    pthread_mutex_unlock(&forks[first]);

    pthread_cond_broadcast(&condition);

    pthread_mutex_unlock(&fork_m);
}

