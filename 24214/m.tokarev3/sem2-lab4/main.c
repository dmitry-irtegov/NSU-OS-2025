#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *my_func(void *arg)
{
    while (1)
    {
        printf("Some text ");
        fflush(stdout);
    }

    return NULL;
}

int main()
{
    pthread_t new_thread;

    if (pthread_create(&new_thread, NULL, my_func, NULL) != 0)
    {
        perror("pthread_create");
        return 1;
    }

    sleep(2);

    if (pthread_cancel(new_thread) != 0)
    {
        perror("pthread_cancel");
        return 1;
    }

    return 0;
}
