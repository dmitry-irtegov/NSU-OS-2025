#include <stdio.h> 
#include <unistd.h> 
#include <sys/types.h>

int main()
{
	uid_t uid = getuid(); 
	uid_t euid = geteuid(); 
	printf("UID: %u\nE-UID: %u\n", uid, euid);
	FILE* file = fopen("test.txt", "r");
	if (file == NULL)
		perror("\nCannot open the file...\n"); 
	else
	{
		printf("\nFile has been opened\n");
		fclose(file);
	}
	setuid(uid); 
	printf("Effective ID has been edites\n");
	file = fopen("test.txt", "r");
	if (file == NULL)
		perror("\nCannot open the file...\n"); 
	else
	{
		printf("\nFile has been opened\n");
		fclose(file);
	}
	printf("\nSuccess...\n"); 
}