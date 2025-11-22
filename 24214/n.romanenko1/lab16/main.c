#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    struct termios new, old;
    int fd = open("/dev/tty", O_RDWR);
    
    if (fd == -1) {
        perror("open error!");
        exit(1);
    }

    if(tcgetattr(fd, &old) != 0)
    {
        perror("tcgetattr error!");
        close(fd);
        exit(1);
    }

    new = old;

    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;

    if(tcsetattr(fd, TCSANOW, &new) != 0)
    {
        perror("tcsetattr error!");
        close(fd);
        exit(1);
    }

    char question[] = "What is 2 + 2? ";
    write(fd, question, strlen(question));

    char answer;
    if(read(fd, &answer, 1) != 1) {
        perror("read error!");
        tcsetattr(fd, TCSANOW, &old);
        close(fd);
        exit(1);
    }

    tcsetattr(fd, TCSANOW, &old);

    write(fd, "\n", 1);

    if(answer == '4')
    {
        write(fd, "Correct!\n", 10);
    }
    else
    {
        write(fd, "Wrong!\n",8);
    }

    close(fd);
    return 0;
}