#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(){
	uid_t id = fork();
	if (id == 0)
	{
	system("cat file.txt");
	return 0;
	}
	printf("Some final string\n");
	wait(NULL);
	printf("Bye\n");
	
	return 0;
}
