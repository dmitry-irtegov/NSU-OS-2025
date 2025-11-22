#include <stdio.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
	pid_t procId = fork(); 
	if (procId == -1) {
		printf("Process creating error\n");
		return 0;
	}
	if (procId == 0) { // we are in subprocess
		char *const argv[3] = {"cat", "test.txt", NULL};
		int execvpCode = execvp("cat", argv);
		if (execvpCode == -1) {
			printf("The sub process finished uncorrectly\n");
			return 0;
		}
	}
	else if (procId != 0) { // we are in the parent 
		int status;
		pid_t waitCode = wait(&status); 
		if (waitCode == -1) {
			printf("Wait system call error\n");
			return 0; 
		} else if (waitCode == 0) {
			printf("Haven't access to subprocesses or no subprocesses finished\n");
			return 0;
		} else if (waitCode == procId){
			printf("Subprocess finished! Now it checking status\n");
			if (WIFEXITED(status))
				printf("Subprocess finished successfully with the status: [%d]\n", status);
			else
				printf("Subprocess finished unsuccessfully with the status: [%d]\n", status);
		}
		printf("==The main process finished==\n"); 
		return 0;
	}
}