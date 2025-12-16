#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define TIME_OUT 5
#define BUF_SIZE 256

int timeout_flag = 0;

void alarm_handler(int sig) {
    timeout_flag = 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s files...\n", argv[0]);
        exit(1);
    }
    
    int fds_entered = argc-1;
    int fds[fds_entered];
    int active_fds[fds_entered];
    int active_fds_count = fds_entered;
    char buf[BUF_SIZE];
    
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            perror(argv[i]);
            exit(1);
        }
        fds[i-1] = fd;
        active_fds[i-1] = 1;
    }

    sigset(SIGALRM, alarm_handler);

    int i = -1;
    while (active_fds_count > 0) {
        i = (i + 1) % fds_entered;

        if (!active_fds[i]) {
            continue;
        }
        
        timeout_flag = 0;

        alarm(TIME_OUT);
        int n = read(fds[i], buf, BUF_SIZE);
        alarm(0);
        
        if (timeout_flag) {
            continue;
        }
        
        if (n > 0) {
            write(STDOUT_FILENO, buf, n);
        } else if (n == 0) {
            close(fds[i]);
            active_fds[i] = 0;
            active_fds_count--;
        } else if (n < 0) {
            perror(argv[i+1]);
            exit(1);
        }
    }
    
    return 0;
}
