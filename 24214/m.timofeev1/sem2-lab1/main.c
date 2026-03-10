#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *thread_func(void *arg)
{
	(void)arg;

	for (int i = 1; i <= 10; i++)
	{
		printf("Child thread: line %d\n", i);
	}

	return NULL;
}

int main(void)
{
	pthread_t thread;
	int rc = pthread_create(&thread, NULL, thread_func, NULL);
	if (rc != 0)
	{
		fprintf(stderr, "pthread_create failed: %d\n", rc);
		return 1;
	}

	for (int i = 1; i <= 10; i++)
	{
		printf("Parent thread: line %d\n", i);
	}

	rc = pthread_join(thread, NULL);
	if (rc != 0)
	{
		fprintf(stderr, "pthread_join failed: %d\n", rc);
		return 1;
	}

	return 0;
}
