#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

int main(){

	if (!isatty(STDIN_FILENO)){
		if (errno == EBADF){
			perror("Failed to check if fildes is tty");
			return -1;
		}
		fprintf(stderr, "Expected terminal device as stdin\n");
		return -1;
	}

	struct termios saved_tty, tty;

	if (tcgetattr(STDIN_FILENO, &tty)){
		perror("Failed to get current terminal attributes");
		return -1;
	}

	saved_tty = tty;

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSANOW, &tty)){
		perror("Failed to set modified terminal attributes");
		return -1;
	}

	printf("Do you like this program? (y/n): ");

	while (1){
		int c = fgetc(stdin);

		if (c == 'y'){
			printf("y\nVery good of you!\n");
			break;
		}
		else if (c == 'n'){
			printf("n\nYou're lying, don't you?\n");
			break;
		}
	}

	if (tcsetattr(STDIN_FILENO, TCSANOW, &saved_tty)){
		perror("Failed to restore terminal attributes, do it yourself or persist in the doomed world you have created");
		return -1;
	}

	return 0;
}
