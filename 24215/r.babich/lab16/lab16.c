#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int main() {
	int fd;
	char ch;
	struct termios tty, savetty;

	fd = open("/dev/tty", O_RDONLY);
	if (fd == -1) {
		perror("Failed to open terminal.");
		return 1;
	}

	if (tcgetattr(fd, &tty) == -1) {
		perror("Failed to get terminal attributes.");
		close(fd);
		return 1;
	}
	savetty = tty;
	tty.c_lflag &= ~(ICANON | ECHO);
	tty.c_cc[VMIN] = 1;
  
	if (tcsetattr(fd, TCSAFLUSH, &tty) == -1) {
		perror("Failed to set modified terminal attributes.");
		close(fd);
		return 1;
	}

	setbuf(stdout, (char*) NULL);
	printf("y/n? ");
	read(fd, &ch, 1);
	printf("%c\n", ch);

	if (tcsetattr(fd, TCSAFLUSH, &savetty) == -1) {
		perror("Failed to restore terminal attributes.");
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}

