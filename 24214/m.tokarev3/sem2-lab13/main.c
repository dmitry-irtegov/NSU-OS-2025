#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define MESSAGE_COUNT 10

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = 0; // 0 - parent, 1 - child

void *child_thread(void *arg)
{
    for (int i = 1; i <= MESSAGE_COUNT; i++)
    {
        pthread_mutex_lock(&mutex);

        while (turn != 1)
        {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Child thread says %d\n", i);

        turn = 0;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main()
{
    pthread_t thread;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&mutex, &attr);
    pthread_cond_init(&cond, NULL);

    if (pthread_create(&thread, NULL, child_thread, NULL) != 0)
    {
        perror("Failed to create thread");
        return 1;
    }

    for (int i = 1; i <= MESSAGE_COUNT; i++)
    {
        pthread_mutex_lock(&mutex);

        while (turn != 0)
        {
            pthread_cond_wait(&cond, &mutex);
        }

        printf("Main thread says %d\n", i);

        turn = 1;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }

    pthread_join(thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    pthread_mutexattr_destroy(&attr);

    return 0;
}
