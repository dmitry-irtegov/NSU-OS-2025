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

    if (id == PHILO - 1) {
        right_fork = 0;
        left_fork = 4;
	
    }

    while (f = food_on_table()) {

	if( id == 1){
    	    sleep(sleep_seconds);
	}

	printf("Philosopher %d: get dish %d\n", id, f);
	get_fork(id, right_fork);
	get_fork(id, left_fork);

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

void get_fork(int phil, int fork){
    pthread_mutex_lock(&forks[fork]);
    printf("Philosopher %d: got fork %d\n", phil, fork);
}

void down_forks(int first, int second){
    pthread_mutex_unlock(&forks[second]);
    pthread_mutex_unlock(&forks[first]);
}
