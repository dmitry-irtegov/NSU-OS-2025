#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 512

int main(){

	int mypipe[2];

	if (pipe(mypipe)){
		perror("Failed to create pipe");
		return -1;
	}

	pid_t child = fork();

	if (child == -1){
		perror("Failed to fork new process");
		return -1;
	}

	// Parent
	if (child){
		if (close(mypipe[1])){
			perror("Failed to close unneeded end of pipe");
			return -1;
		}

		char buff[BUFFER_SIZE];
		ssize_t rc;
		while (1){
			rc = read(mypipe[0], buff, BUFFER_SIZE);
			if (rc <= 0){
				break;
			}

			for (size_t i = 0; i < (size_t) rc; i++){
				buff[i] = toupper(buff[i]);
			}

			if (fwrite(buff, 1, (size_t) rc, stdout) != (size_t) rc){
				perror("Failed to write received data");
				return -1;
			}
		}

		if (rc == -1){
			perror("Failed to read from pipe");
			return -1;
		}


		wait(NULL);

		return 0;
	}

	// Child
	else{
		if (close(mypipe[0])){
			perror("Failed to close unneeded end of pipe");
			return -1;
		}

		char msg[] = "I wRiTe SmTh To ThE pIpE\n";

		if (write(mypipe[1], msg, sizeof(msg)) != sizeof(msg)){
			perror("Failed to write message to pipe");
			return -1;
		}

		return 0;
	}

}

