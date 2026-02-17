#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t child_mutex;
pthread_mutex_t parent_mutex;
pthread_mutex_t start_mutex;

void *child_thread(void *atr)
{
    pthread_mutex_lock(&child_mutex);

    for (int i = 1; i <= 10; i++)
    {
        pthread_mutex_lock(&parent_mutex);
        pthread_mutex_unlock(&child_mutex);

        printf("New thread says %d\n", i);

        pthread_mutex_lock(&start_mutex);
        pthread_mutex_unlock(&parent_mutex);
        pthread_mutex_lock(&child_mutex);
        pthread_mutex_unlock(&start_mutex);
    }

    return NULL;
}

int main()
{
    pthread_t new_thread;
    pthread_mutexattr_t attr;

    if (pthread_mutexattr_init(&attr))
    {
        perror("mutexattr init error");
        return 1;
    }

    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK))
    {
        perror("mutexattr set type error");
        return 1;
    }

    if (pthread_mutex_init(&start_mutex, &attr) ||
        pthread_mutex_init(&child_mutex, &attr) ||
        pthread_mutex_init(&parent_mutex, &attr))
    {
        perror("mutex init error");
        return 1;
    }

    pthread_mutex_lock(&parent_mutex);

    if (pthread_create(&new_thread, NULL, child_thread, NULL))
    {
        perror("pthread_create error");
        return 1;
    }

    usleep(100);

    for (int i = 1; i <= 10; i++)
    {
        printf("Main thread says %d\n", i);

        pthread_mutex_lock(&start_mutex);
        pthread_mutex_unlock(&parent_mutex);
        pthread_mutex_lock(&child_mutex);
        pthread_mutex_unlock(&start_mutex);
        pthread_mutex_lock(&parent_mutex);
        pthread_mutex_unlock(&child_mutex);
    }

    if (pthread_join(new_thread, NULL))
    {
        perror("thread join error");
        return 1;
    }

    pthread_mutex_destroy(&start_mutex);
    pthread_mutex_destroy(&child_mutex);
    pthread_mutex_destroy(&parent_mutex);
    pthread_mutexattr_destroy(&attr);

    return 0;
}
