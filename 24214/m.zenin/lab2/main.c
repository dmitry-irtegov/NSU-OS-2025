#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(){

	time_t t = time(NULL);
	if (t == -1){
		perror("Seems like you live in a very far future");
		return -1;
	}

	if (putenv("TZ=:America/Los_Angeles")){
		perror("Failed to set TZ env variable");
		return -1;
	}

	char *output = ctime(&t);
	if (!output){
		perror("Failed to get current time in California as a string");
		return -1;
	}

	printf("Time in California is: %s", output);

	return 0;
}
