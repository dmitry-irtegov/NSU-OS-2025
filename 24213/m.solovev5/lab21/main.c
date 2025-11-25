#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int finish = 0;
unsigned long long cnt = 0;

void on_sigquit(int sig) {
	signal(sig, SIG_IGN);

	if (sig == SIGQUIT) {
		finish = 1;
	}

	signal(sig, on_sigquit);
}

void on_sigint(int sig) {
	signal(sig, SIG_IGN);

	if (sig == SIGINT) {
		cnt = cnt + 1;
		write(STDOUT_FILENO, "\a", 1);
	}

	signal(sig, on_sigint);
}

int main() {
	signal(SIGINT, on_sigint);
	signal(SIGQUIT, on_sigquit);

	while (!finish) {
		pause();
	}

	printf("\nBeeps: %llu\n", cnt);
	exit(0);
}
