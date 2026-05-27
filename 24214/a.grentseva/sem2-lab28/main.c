#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 4096
#define SCREEN_LINES 25
#define SCREEN_WIDTH 80

struct termios oldt;

void restore_terminal(void)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1)
    {
        perror("tcsetattr");
    }
}

void check_signal(int sig)
{
    (void)sig;
    exit(0);  
}

int setup_terminal(void)
{
    struct termios newt;

    if (!isatty(STDIN_FILENO))
        return -1;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1)
        return -1;
    if (atexit(restore_terminal) != 0)
        return -1;
    
    if (signal(SIGINT, check_signal) == SIG_ERR)
        return -1;
    if (signal(SIGTERM, check_signal) == SIG_ERR)
        return -1;

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s http://host/path\n", argv[0]);
        return 1;
    }

    char host[256];
    char path[1024] = "/";

    if (sscanf(argv[1], "http://%255[^/]%1023[^\n]", host, path) < 1)
    {
        return 1;
    }

    struct addrinfo hints = {0};
    struct addrinfo *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "80", &hints, &res) != 0)
    {
        return 1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        freeaddrinfo(res);
        return 1;
    }
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        freeaddrinfo(res);
        close(sockfd);
        return 1;
    }
    freeaddrinfo(res);

    char request[2048];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n\r\n",
             path, host);

    if (send(sockfd, request, strlen(request), 0) == -1)
    {
        close(sockfd);
        return 1;
    }

    if (setup_terminal() != 0)
    {
        close(sockfd);
        return 1;
    }
    char buffer[BUF_SIZE];
    int bytes_in_buffer = 0;
    int buffer_pos = 0;

    bool headers_skipped = false;
    bool paused = false;
    bool connection_closed = false;

    int lines = 0;
    int cols = 0;

    while (1)
    {
        if (buffer_pos > 0)
        {
            int remain = bytes_in_buffer - buffer_pos;
            memmove(buffer, buffer + buffer_pos, remain);
            bytes_in_buffer = remain;
            buffer_pos = 0;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (!connection_closed && bytes_in_buffer < BUF_SIZE)
        {
            FD_SET(sockfd, &readfds);
        }

        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1)
        {
            close(sockfd);
            return 1;
        }

        if (!connection_closed && FD_ISSET(sockfd, &readfds))
        {
            int n = recv(sockfd, buffer + bytes_in_buffer, BUF_SIZE - bytes_in_buffer, 0);
            if (n > 0)
            {
                bytes_in_buffer += n;
            }
            else if (n == 0)
            {
                connection_closed = true;
            }
            else
            {
                close(sockfd);
                return 1;
            }
        }

        if (!headers_skipped)
        {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body)
            {
                int header_size = body - buffer + 4;
                memmove(buffer, buffer + header_size, bytes_in_buffer - header_size);
                bytes_in_buffer -= header_size;
                headers_skipped = true;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            char c;
            if (read(STDIN_FILENO, &c, 1) <= 0)
            {
                break;
            }

            if (paused && c == ' ')
            {
                paused = false;
                lines = 0;
                printf("\r                               \r");
                fflush(stdout);
            }
        }

        while (!paused && buffer_pos < bytes_in_buffer)
        {
            char c = buffer[buffer_pos];
            putchar(c);
            buffer_pos++;
            cols++;

            if (c == '\n')
            {
                lines++;
                cols = 0;
            }

            if (cols >= SCREEN_WIDTH)
            {
                lines++;
                cols = 0;
            }

            if (lines >= SCREEN_LINES)
            {
                paused = true;
                printf("\nPress space to scroll down");
                fflush(stdout);
                break;
            }
        }

        fflush(stdout);

        if (connection_closed && buffer_pos >= bytes_in_buffer)
        {
            break;
        }
    }

    close(sockfd);
    return 0;
}