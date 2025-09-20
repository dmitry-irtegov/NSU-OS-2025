#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
	if (setenv("TZ", ":America/Los_Angeles", 1) != 0)
	{
		perror("Failed to set env");
		exit(1);
	}

	tzset();

	time_t now = time(NULL);
	if (now == -1)
	{
		perror("Failed to get time");
		exit(1);
	}
	char *time = ctime(&now);

	if (time == NULL)
	{
		perror("Failed to get time");
		exit(1);
	}

	printf("%s", time);
	exit(0);
}
