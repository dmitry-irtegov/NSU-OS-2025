#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50
#define MAX_WAIT 1000
#define MAX_RETRIES 50

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];
pthread_mutex_t foodlock;
pthread_mutex_t printlock;

int sleep_seconds = 0;

int meals_eaten[PHILO] = {0};
pthread_mutex_t stats_lock;

void *philosopher(void *num);
int food_on_table(void);
void acquire_forks(int phil, int left, int right);
void get_fork(int phil, int fork, char *hand);
void down_forks(int f1, int f2);
void safe_printf(const char *format, ...);

int main(int argn, char **argv)
{
    int i;

    if (argn == 2)
        sleep_seconds = atoi(argv[1]);

    pthread_mutex_init(&foodlock, NULL);
    pthread_mutex_init(&printlock, NULL);
    pthread_mutex_init(&stats_lock, NULL);

    for (i = 0; i < PHILO; i++)
        pthread_mutex_init(&forks[i], NULL);

    for (i = 0; i < PHILO; i++)
        pthread_create(&phils[i], NULL, philosopher, (void *)(long)i);

    for (i = 0; i < PHILO; i++)
        pthread_join(phils[i], NULL);

    safe_printf("\n=== STATISTICS ===\n");
    for (i = 0; i < PHILO; i++)
    {
        safe_printf("Philosopher %d ate %d meals\n", i, meals_eaten[i]);
    }

    pthread_mutex_destroy(&foodlock);
    pthread_mutex_destroy(&printlock);
    pthread_mutex_destroy(&stats_lock);
    for (i = 0; i < PHILO; i++)
        pthread_mutex_destroy(&forks[i]);

    return 0;
}

void acquire_forks(int phil, int left, int right)
{
    int first = left < right ? left : right;
    int second = left < right ? right : left;
    pthread_mutex_lock(&forks[first]);
    safe_printf("Philosopher %d: got fork %d (first)\n", phil, first);
    pthread_mutex_lock(&forks[second]);
    safe_printf("Philosopher %d: got fork %d (second)\n", phil, second);
}

void *philosopher(void *num)
{
    long id = (long)num;
    int left_fork = id;
    int right_fork = (id + 1) % PHILO;
    int dish;
    int meals = 0;

    safe_printf("Philosopher %ld sitting down to dinner.\n", id);

    while ((dish = food_on_table()) > 0)
    {
        safe_printf("Philosopher %ld: get dish %d.\n", id, dish);

        acquire_forks(id, left_fork, right_fork);
        safe_printf("Philosopher %ld: eating dish %d.\n", id, dish);
        usleep(DELAY * (FOOD - dish + 1));
        down_forks(left_fork, right_fork);
        meals++;
        safe_printf("Philosopher %ld: finished eating dish %d.\n", id, dish);
    }

    pthread_mutex_lock(&stats_lock);
    meals_eaten[id] = meals;
    pthread_mutex_unlock(&stats_lock);

    safe_printf("Philosopher %ld is done eating (ate %d meals).\n", id, meals);
    return NULL;
}

void get_fork(int phil, int fork, char *hand)
{
    pthread_mutex_lock(&forks[fork]);
    safe_printf("Philosopher %d: got %s fork %d\n", phil, hand, fork);
}

void down_forks(int f1, int f2)
{
    pthread_mutex_unlock(&forks[f1]);
    pthread_mutex_unlock(&forks[f2]);
}

int food_on_table(void)
{
    static int food = FOOD;
    int myfood;

    pthread_mutex_lock(&foodlock);
    if (food > 0)
    {
        food--;
        myfood = FOOD - food;
    }
    else
    {
        myfood = 0;
    }
    pthread_mutex_unlock(&foodlock);

    return myfood;
}

void safe_printf(const char *format, ...)
{
    va_list args;
    pthread_mutex_lock(&printlock);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    pthread_mutex_unlock(&printlock);
}