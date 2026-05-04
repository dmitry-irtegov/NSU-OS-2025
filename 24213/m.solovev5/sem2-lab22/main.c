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
#include <string.h>
#include <unistd.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t getting_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t getting_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
void *philosopher(void *id);
int food_on_table();
void get_forks(int, int, int);
void down_forks(int, int);
pthread_mutex_t foodlock;

int main(int argn, char **argv) {
  int i;

  pthread_mutex_init(&foodlock, NULL);
  for (i = 0; i < PHILO; i++)
    pthread_mutex_init(&forks[i], NULL);
  for (i = 0; i < PHILO; i++)
    pthread_create(&phils[i], NULL, philosopher, (void *)i);
  for (i = 0; i < PHILO; i++)
    pthread_join(phils[i], NULL);
  int code = pthread_mutex_destroy(&getting_mutex);
  if (code != 0) {
    char buf[256];
    strerror_r(code, buf, sizeof buf);
    fprintf(stderr, "mutex_destroy: %s", buf);
    exit(1);
  }
  code = pthread_cond_destroy(&getting_cond);
  if (code != 0) {
    char buf[256];
    strerror_r(code, buf, sizeof buf);
    fprintf(stderr, "cond_destroy: %s", buf);
    exit(1);
  }
  return 0;
}

void *philosopher(void *num) {
  int id;
  int left_fork, right_fork, f;

  id = (int)num;
  printf("Philosopher %d sitting down to dinner.\n", id);
  right_fork = id;
  left_fork = id + 1;

  /* Wrap around the forks. */
  if (left_fork == PHILO)
    left_fork = 0;

  while (f = food_on_table()) {
    printf("Philosopher %d: get dish %d.\n", id, f);
    get_forks(id, right_fork, left_fork);

    printf("Philosopher %d: eating.\n", id);
    usleep(DELAY * (FOOD - f + 1));
    down_forks(left_fork, right_fork);
  }
  printf("Philosopher %d is done eating.\n", id);
  return (NULL);
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

void get_forks(int phil, int f1, int f2) {
  pthread_mutex_lock(&getting_mutex);
  for (;;) {
    int code = pthread_mutex_trylock(&forks[f1]);
    if (code == 0) {
      code = pthread_mutex_trylock(&forks[f2]);
      if (code == 0) {
        printf("Philosopher %d: got fork %d and fork %d\n", phil, f1, f2);
        break;
      } else {
        pthread_mutex_unlock(&forks[f1]);
      }
    }
    pthread_cond_wait(&getting_cond, &getting_mutex);
  }
  pthread_mutex_unlock(&getting_mutex);
}

void down_forks(int f1, int f2) {
  pthread_mutex_lock(&getting_mutex);

  pthread_mutex_unlock(&forks[f1]);
  pthread_mutex_unlock(&forks[f2]);
  pthread_cond_broadcast(&getting_cond);

  pthread_mutex_unlock(&getting_mutex);
}
