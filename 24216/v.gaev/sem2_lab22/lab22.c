#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#define PHILO 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[PHILO];
pthread_t phils[PHILO];

// Глобальные примитивы синхронизации для атомарного захвата
pthread_mutex_t global_forks_mutex;
pthread_cond_t forks_cv;
pthread_mutex_t foodlock;

int sleep_seconds = 0;

void *philosopher(void *id);
int food_on_table();
void get_forks(int phil, int left, int right);
void down_forks(int f1, int f2);

int main(int argn, char **argv) {
    int i;

    if (argn == 2)
        sleep_seconds = atoi(argv[1]);

    // Инициализация мьютексов и условной переменной
    pthread_mutex_init(&foodlock, NULL);
    pthread_mutex_init(&global_forks_mutex, NULL);
    pthread_cond_init(&forks_cv, NULL);
    
    for (i = 0; i < PHILO; i++)
        pthread_mutex_init(&forks[i], NULL);
        
    for (i = 0; i < PHILO; i++)
        pthread_create(&phils[i], NULL, philosopher, (void *)(intptr_t)i);
        
    for (i = 0; i < PHILO; i++)
        pthread_join(phils[i], NULL);

    // Очистка ресурсов (хороший тон для проектов на C/Linux)
    pthread_mutex_destroy(&global_forks_mutex);
    pthread_cond_destroy(&forks_cv);
    pthread_mutex_destroy(&foodlock);
    for (i = 0; i < PHILO; i++)
        pthread_mutex_destroy(&forks[i]);

    return 0;
}

void *philosopher(void *num) {
    int id = (int)(intptr_t)num;
    int left_fork, right_fork, f;

    printf("Philosopher %d sitting down to dinner.\n", id);
    right_fork = id;
    left_fork = id + 1;
 
    if (left_fork == PHILO)
        left_fork = 0;
 
    while ((f = food_on_table())) {
        if (id == 1)
            sleep(sleep_seconds);

        printf("Philosopher %d: get dish %d.\n", id, f);
        
        // Пытаемся взять обе вилки сразу
        get_forks(id, left_fork, right_fork);

        printf("Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));
        
        // Освобождаем вилки
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

void get_forks(int phil, int left, int right) {
    // Блокируем глобальный мьютекс для проверки состояния вилок
    pthread_mutex_lock(&global_forks_mutex);
    
    while (1) {
        // Пробуем захватить левую вилку
        if (pthread_mutex_trylock(&forks[left]) == 0) {
            // Если левая захвачена, пробуем правую
            if (pthread_mutex_trylock(&forks[right]) == 0) {
                // Обе вилки успешно захвачены
                printf("Philosopher %d: got left fork %d and right fork %d\n", phil, left, right);
                break; 
            } else {
                // Правую взять не удалось — освобождаем левую, чтобы не спровоцировать дэдлок
                pthread_mutex_unlock(&forks[left]);
            }
        }
        // Если взять обе вилки не вышло, засыпаем и ждем сигнала от других философов.
        // pthread_cond_wait атомарно отпускает global_forks_mutex и усыпляет поток.
        pthread_cond_wait(&forks_cv, &global_forks_mutex);
    }
    
    // Снимаем блокировку глобального мьютекса
    pthread_mutex_unlock(&global_forks_mutex);
}

void down_forks(int f1, int f2) {
    // Захватываем глобальный мьютекс перед тем, как положить вилки и разбудить соседей
    pthread_mutex_lock(&global_forks_mutex);
    
    pthread_mutex_unlock(&forks[f1]);
    pthread_mutex_unlock(&forks[f2]);
    
    // Оповещаем ВСЕ спящие потоки (широковещательно), что состояние изменилось
    pthread_cond_broadcast(&forks_cv);
    
    pthread_mutex_unlock(&global_forks_mutex);
}
