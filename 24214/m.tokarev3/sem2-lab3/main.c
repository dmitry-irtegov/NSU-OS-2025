#include <stdio.h>
#include <pthread.h>

void *StringPrinter(void *arg)
{
    char **strings = (char **)arg;

    int i = 0;
    while (strings[i])
    {
        printf("%s\n", strings[i++]);
    }

    return NULL;
}

int main()
{
    int numOfThreads = 4;

    pthread_t threads[numOfThreads];
    char *list1[] = {"Apple", "Banana", "Orange", "Watermelon", NULL};
    char *list2[] = {"Red", "Green", "Blue", "White", NULL};
    char *list3[] = {"C", "C++", "Go", "Python", NULL};
    char *list4[] = {"Winter", "Spring", "Summer", "Fall", NULL};

    char **strings[] = {list1, list2, list3, list4};

    for (int i = 0; i < numOfThreads; i++)
    {
        if (pthread_create(&threads[i], NULL, StringPrinter, strings[i]) != 0)
        {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < numOfThreads; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            perror("pthread_join");
            return 1;
        }
    }

    return 0;
}
