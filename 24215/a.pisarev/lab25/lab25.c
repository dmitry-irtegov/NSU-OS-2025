#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <ctype.h>

#define MAX_BUF_SIZE 30

int main(int argc, char *argv[ ])
{
	int fd[2];
	if (pipe(fd)== -1)
		perror("Pipe");
	
	pid_t pid = fork();
	if (pid==0){
		close(fd[0]);
		char* msg = "I love ice cream!\n";
		if (write(fd[1],msg, strlen(msg)) == -1)
			perror("write");
		return 0;
	}
	close(fd[1]);
	char buf[MAX_BUF_SIZE];
	wait(NULL);
	if (read(fd[0], buf, MAX_BUF_SIZE)==-1)
		perror("read");
	close(fd[0]);
	char upBuf[MAX_BUF_SIZE];
	int i;
	for (i =0; i < MAX_BUF_SIZE;i++){
		if (buf[i]!='\0'){
			upBuf[i] = toupper(buf[i]);
		}
		else{
			upBuf[i]='\0';
			break;
		}
	}
	printf("String:%s",upBuf);
	return 0;
}
