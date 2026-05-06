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
void *philosopher (void *id);
int food_on_table ();
void get_forks (int, int, int);
void down_forks (int, int);
pthread_mutex_t foodlock;
pthread_mutex_t forklock;
pthread_cond_t fork_cond;
int sleep_seconds = 0;

int
main (int argn,
      char **argv)
{
  int i;

  if (argn == 2)
    sleep_seconds = atoi (argv[1]);

  pthread_cond_init (&fork_cond, NULL);
  pthread_mutex_init (&forklock, NULL);
  pthread_mutex_init (&foodlock, NULL);
  for (i = 0; i < PHILO; i++)
    pthread_mutex_init (&forks[i], NULL);
  for (i = 0; i < PHILO; i++)
    pthread_create (&phils[i], NULL, philosopher, (void *)i);
  for (i = 0; i < PHILO; i++)
    pthread_join (phils[i], NULL);
  return 0;
}

void *
philosopher (void *num)
{
  int id;
  int left_fork, right_fork, f;

  id = (int)num;
  printf ("Philosopher %d sitting down to dinner.\n", id);
  right_fork = id;
  left_fork = id + 1;
 
  /* Wrap around the forks. */
  if (left_fork == PHILO)
    left_fork = 0;
 
  while (1) {

    /* Thanks to philosophers #1 who would like to 
     * take a nap before picking up the forks, the other
     * philosophers may be able to eat their dishes and 
     * not deadlock.
     */
    if (id == 1)
      sleep (sleep_seconds);

    printf ("Philosopher %d: get dish %d.\n", id, f);
    
    get_forks (id, right_fork, left_fork);
    printf ("Philosopher %d: eating.\n", id);
    usleep (DELAY * (FOOD - f + 1));
    down_forks (left_fork, right_fork);
    usleep(DELAY);
  }
  printf ("Philosopher %d is done eating.\n", id);
  return (NULL);
}

int
food_on_table ()
{
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

void
get_forks (int phil,
          int left_fork, int right_fork)
{
  pthread_mutex_lock (&forklock);
  int result = 1;
  while (result != 0) {
        if (pthread_mutex_trylock(&forks[right_fork]) == 0) {
            if (pthread_mutex_trylock(&forks[left_fork]) == 0) {
                result = 0;
            } else {
                pthread_mutex_unlock(&forks[right_fork]);
            }
        }
        if (result != 0) {
            pthread_cond_wait(&fork_cond, &forklock);
        }
    }
  pthread_mutex_unlock (&forklock);
  printf ("Philosopher %d: got forks %d %d\n", phil, right_fork, left_fork);
}

void
down_forks (int f1,
            int f2)
{
  pthread_mutex_lock(&forklock);
  pthread_mutex_unlock (&forks[f1]);
  pthread_mutex_unlock (&forks[f2]);
  pthread_cond_broadcast(&fork_cond); 
  pthread_mutex_unlock(&forklock);
}
