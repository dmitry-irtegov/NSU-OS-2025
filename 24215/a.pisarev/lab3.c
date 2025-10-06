#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
int main()
{
	 uid_t uid = getuid();
	 uid_t euid = geteuid();
	 fprintf(stdout, "your uid:%d\nyour euid:%d\n", uid,euid);
	FILE *fd;
	if((fd = fopen("secret.txt","r")) == NULL){
		perror("fopen1");
	 }
	 else{
		fclose(fd);
	}
	
	euid = geteuid();
	if(setuid(euid)==-1){
		perror("setuid\n");
	}
	
	 uid = getuid();
	 
	 fprintf(stdout, "your uid:%d\nyour euid:%d\n", uid,euid);
	if((fd = fopen("secret.txt","r")) == NULL){
		perror("fopen2");
	 }
	 else{
	 	fclose(fd);
	 }
	 return 0;
}

