#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *print_10(void *arg)
{
    (void)arg;
    for (int i = 0; i < 10; i++)
    {
        fprintf(stderr, "child\n");
    }
    return NULL;
}

int main()
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, print_10, NULL))
    {
        fprintf(stderr, "pthread_create failed\n");
        return 1;
    }

    for (int i = 0; i < 10; i++)
    {
        fprintf(stderr, "parent\n");
    }

    if (pthread_join(thread, NULL))
    {
        fprintf(stderr, "pthread_join failed\n");
        return 1;
    }

    return 0;
}