#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

int main(){
    struct termios tty, saved_tty;
    char ans;

    int filed = open("/dev/tty", O_RDONLY);
    
    if (filed == -1){
        perror("Error opening terminal");
        exit(EXIT_FAILURE);
    }
    
    tcgetattr(filed, &tty);

    saved_tty = tty;
    tty.c_lflag &= ~(ICANON);
    tty.c_cc[VMIN] = 1;
    tcsetattr(filed, TCSAFLUSH, &tty);

    setbuf(stdout, (char*)NULL);

    printf("question? ");
    if (read(filed, &ans, 1) == -1){
        perror("error reading from terminal");
        tcsetattr(filed, TCSAFLUSH, &saved_tty);
        exit(EXIT_FAILURE);
    }
    printf("\nok\n");
    tcsetattr(filed, TCSAFLUSH, &saved_tty);

    close(filed);

    exit(EXIT_SUCCESS);
}