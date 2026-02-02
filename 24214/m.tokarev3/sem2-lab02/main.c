#include <pthread.h>
#include <stdio.h>

void *my_thread(void *arg)
{
    for (int i = 1; i <= 10; i++)
    {
        printf("Thread says %d\n", i);
    }
    return NULL;
}

int main()
{
    pthread_t new_thread;

    if (pthread_create(&new_thread, NULL, my_thread, NULL) != 0)
    {
        perror("pthread_create");
        return 1;
    }

    if (pthread_join(new_thread, NULL) != 0) // wait new_thread
    {
        perror("pthread_join");
        return 1;
    }

    for (int i = 1; i <= 10; i++)
    {
        printf("Main thread says %d\n", i);
    }

    return 0;
}
