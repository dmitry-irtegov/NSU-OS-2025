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

    if (pthread_detach(new_thread) != 0)
    {
        perror("pthread_detach");
        return 1;
    }

    for (int i = 1; i <= 10; i++)
    {
        printf("Main thread says %d\n", i);
    }

    pthread_exit(NULL);
}
