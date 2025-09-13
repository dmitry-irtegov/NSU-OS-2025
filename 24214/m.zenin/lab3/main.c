#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(){
	printf("Real UID: %ju\n", (uintmax_t)getuid());
	printf("Effective UID: %ju\n\n", (uintmax_t)geteuid());

	FILE *file = fopen("iamfile", "r");
	if (!file){
		perror("First try failed");
	}
	else{
		fclose(file);
	}

	if (seteuid(getuid())){
		perror("Failed to set EUID to UID");
		return -1;
	}

	printf("Real UID: %ju\n", (uintmax_t)getuid());
	printf("Effective UID: %ju\n\n", (uintmax_t)geteuid());

	file = fopen("iamfile", "r");
	if (!file){
		perror("Second try failed");
	}
	else{
		fclose(file);
	}

	return 0;
}
