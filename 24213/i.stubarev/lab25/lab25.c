#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096

int main()
{
    int pipefd[2];
    char buf[BUFFER_SIZE];
    ssize_t br;

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return 1;
    }

    switch (fork())
    {
    case -1:
        perror("fork");
        return 1;
    case 0:
        close(pipefd[1]);
        while ((br = read(pipefd[0], buf, BUFFER_SIZE)) > 0)
        {
            for (ssize_t i = 0; i < br; i++)
            {
                buf[i] = toupper((unsigned char)buf[i]);
            }

            if (write(STDOUT_FILENO, buf, br) == -1)
            {
                perror("write in child");
                return 1;
            }
        }
        if (br == -1)
        {
            perror("read in child");
            return 1;
        }
        return 0;
    default:
        close(pipefd[0]);
        int rf = 0;
        while ((br = read(STDIN_FILENO, buf, BUFFER_SIZE)) > 0)
        {
            if (write(pipefd[1], buf, br) != br)
            {
                perror("write");
                rf = 1;
                break;
            }
        }
        if (br == -1)
        {
            perror("read");
            rf = 1;
        }
        close(pipefd[1]);
        if (wait(NULL) == -1)
        {
            perror("wait");
            rf = 1;
        }
        return rf;
    }
}
