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

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
void *philosopher(void *id);
int food_on_table();
int try_get_fork(int, int, char *);
void get_fork(int, int, char *);
void down_fork(int);
pthread_mutex_t foodlock, forkslock;
pthread_cond_t forks_released;

int sleep_seconds = 0;

int main(int argc, char **argv) {
	int i;

	if (argc == 2) 
		sleep_seconds = atoi(argv[1]);

	pthread_mutex_init(&foodlock, NULL);
	pthread_mutex_init(&forkslock, NULL);
	pthread_cond_init(&forks_released, NULL);
	
	for (i = 0; i < PHILO; i++)
		pthread_mutex_init(&forks[i], NULL);
	for (i = 0; i < PHILO; i++)
		pthread_create(&phils[i], NULL, philosopher, (void *)i);
	for (i = 0; i < PHILO; i++)
		pthread_join(phils[i], NULL);
	return 0;
}

void *philosopher(void *num) {
	int id;
	int left_fork, right_fork, food;

	id = (int)num;
	printf ("Philosopher %d sitting down to dinner.\n", id);
	right_fork = id;
	left_fork = id + 1;

	/* Wrap around the forks. */
	if (left_fork == PHILO) {
		left_fork = 0;
	}

	while ((food = food_on_table())) {

		pthread_mutex_lock(&forkslock);
		printf("Philosopher %d: get dish %d.\n", id, food);
		while (1) {
			get_fork(id, right_fork, "right");
			if (try_get_fork(id, left_fork, "left")) {
				break;
			} else {
				down_fork(right_fork);
				pthread_cond_wait(&forks_released, &forkslock);
			}
		}

		pthread_mutex_unlock(&forkslock);

		printf("Philosopher %d: eating.\n", id);
		usleep(DELAY * (FOOD - food + 1));
		down_fork(left_fork);
		down_fork(right_fork);

		pthread_mutex_lock(&forkslock);
		pthread_cond_broadcast(&forks_released);
		pthread_mutex_unlock(&forkslock);
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

int try_get_fork(int phil, int fork, char *hand) {
	if (pthread_mutex_trylock(&forks[fork]) != 0) {
		fprintf(stderr, "Failed mutex trylock");
		return 0;
	}
	printf("Philosopher %d: got %s fork %d\n", phil, hand, fork);
	return 1;
}

void get_fork(int phil, int fork, char *hand) {
	pthread_mutex_lock(&forks[fork]);
	printf("Philosopher %d: got %s fork %d\n", phil, hand, fork);
}

void down_fork(int fork) {
	pthread_mutex_unlock(&forks[fork]);
}
