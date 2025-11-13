#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){
	uid_t id = fork();
	if (id == 0)
	{
		execvp(argv[1],&argv[1]);
		perror("execvp");
		return 1;
	}

	
	int wstatus;
	if (waitpid(id,&wstatus,0)== -1)
	{
		perror("waitpid");
		return 1;
	}
	if (WIFSIGNALED(wstatus))
		printf("Process exited by signal, signal number=%d.",WTERMSIG(wstatus));
	if (WIFEXITED(wstatus))
		printf("Exit status=%d.",WEXITSTATUS(wstatus));
	
	return 0;
}
