#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main() {
  struct termios old_tio, new_tio;
  char c;

  if (tcgetattr(STDIN_FILENO, &old_tio)== -1){
		perror("tcgetattr to old tio");
		return 1;
	}
  new_tio = old_tio;

	new_tio.c_lflag &= ~ICANON;
	new_tio.c_lflag |= ECHO;

    new_tio.c_cc[VMIN] = 1;              
	if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio)==-1)
		perror("tcsetattr to new tio");

	printf("This is: ");
	fflush(stdout);

	if (read(STDIN_FILENO, &c, 1)==-1)
		perror("read");

  if (tcsetattr(STDIN_FILENO, TCSANOW, &old_tio)==-1)
		perror("tcsetattr to old_tio");

  printf("\nDone.\n");
	fflush(stdout);
    return 0;
}
