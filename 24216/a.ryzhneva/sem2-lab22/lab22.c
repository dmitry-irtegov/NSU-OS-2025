#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];

pthread_mutex_t getting_forks_mx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t getting_forks_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t foodlock = PTHREAD_MUTEX_INITIALIZER;

void *philosopher(void *id);
int food_on_table();
void down_forks(int, int);
void get_forks(int, int, int);

void check_code(int code, const char* name_prog, const char* action) {
    if (code != 0) {
        char buf[256];
        strerror_r(code, buf, sizeof(buf));
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(EXIT_FAILURE);
    }
}

int main (int argc, char **argv)
{
  int i;
  int code;

  pthread_mutex_init(&foodlock, NULL);

  for (i = 0; i < PHILO; i++) {
    pthread_mutex_init(&forks[i], NULL);
  }
  for (i = 0; i < PHILO; i++) {
    code = pthread_create(&phils[i], NULL, philosopher, (void*)(intptr_t)i);
    check_code(code, argv[0], "creating thread");
  }
  for (i = 0; i < PHILO; i++) {
    code = pthread_join(phils[i], NULL);
    check_code(code, argv[0], "joining thread");
  }

  code = pthread_mutex_destroy(&foodlock);
  check_code(code, argv[0], "destroying foodlock mutex");
  code = pthread_mutex_destroy(&getting_forks_mx);
  check_code(code, argv[0], "destroying getting_forks_mx mutex");
  code = pthread_cond_destroy(&getting_forks_cond);
  check_code(code, argv[0], "destroying getting_forks_cond mutex");

  for (i = 0; i < PHILO; i++) {
    code = pthread_mutex_destroy(&forks[i]);
    check_code(code, argv[0], "destroying fork mutex");
  }

  return 0;
}

void* philosopher (void *num)
{
  int id;
  int left_fork, right_fork, f;

  id = (int)(intptr_t)num;
  printf ("Philosopher %d sitting down to dinner.\n", id);
  right_fork = id;
  left_fork = id + 1;

  if (left_fork == PHILO) {
    left_fork = 0;
  }

  while ((f = food_on_table())) {

    printf("Philosopher %d: get dish %d.\n", id, f);

    get_forks(id, left_fork, right_fork);

    printf("Philosopher %d: eating.\n", id);
    usleep(DELAY * (FOOD - f + 1));

    down_forks(left_fork, right_fork);
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

void get_forks(int phil, int fork1, int fork2) {
  int res;

  pthread_mutex_lock(&getting_forks_mx);
  do {
    res = pthread_mutex_trylock(&forks[fork1]);
    if (res == 0) {
      res = pthread_mutex_trylock(&forks[fork2]);
      if (res != 0) {
        pthread_mutex_unlock(&forks[fork1]);
      } else {
        printf("Philosopher %d got forks %d %d.\n", phil, fork1, fork2);
      }
    }

    if (res != 0) {
      pthread_cond_wait(&getting_forks_cond, &getting_forks_mx);
    }
  } while(res);

  pthread_mutex_unlock(&getting_forks_mx);
}

void down_forks (int f1, int f2) {
  pthread_mutex_lock(&getting_forks_mx);

  pthread_mutex_unlock (&forks[f1]);
  pthread_mutex_unlock (&forks[f2]);
  
  pthread_cond_broadcast(&getting_forks_cond);
  pthread_mutex_unlock(&getting_forks_mx);
}