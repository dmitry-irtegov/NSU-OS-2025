#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t g_beep_count = 0;

#define IGNORE_ERR_MSG   "error: cannot ignore signal\n"
#define RESTORE_ERR_MSG  "error: cannot restore handler\n"
#define BELL_CHAR        "\a"

static void handle_sigint(int sig) {
    if (signal(sig, SIG_IGN) == SIG_ERR) {
        write(STDERR_FILENO, IGNORE_ERR_MSG, sizeof(IGNORE_ERR_MSG) - 1);
        _exit(EXIT_FAILURE);
    }

    if (write(STDOUT_FILENO, BELL_CHAR, 1) == -1) {
        _exit(EXIT_FAILURE);
    }

    g_beep_count++;

    if (signal(sig, handle_sigint) == SIG_ERR) {
        write(STDERR_FILENO, RESTORE_ERR_MSG, sizeof(RESTORE_ERR_MSG) - 1);
        _exit(EXIT_FAILURE);
    }
}

#define COUNT_BUF_SIZE 64

static void handle_sigquit(int sig) {
    (void)sig;

    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
        write(STDERR_FILENO, IGNORE_ERR_MSG, sizeof(IGNORE_ERR_MSG) - 1);
        _exit(EXIT_FAILURE);
    }

    char buf[COUNT_BUF_SIZE];
    int len = snprintf(buf, COUNT_BUF_SIZE, "\nbeep count: %d\n", g_beep_count);
    if (len < 0) {
        _exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, buf, (size_t)len);
    _exit(EXIT_SUCCESS);
}

int main(void) {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("cannot set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGQUIT, handle_sigquit) == SIG_ERR) {
        perror("cannot set SIGQUIT handler");
        exit(EXIT_FAILURE);
    }

    setbuf(stdout, NULL);
    printf("Введите Ctrl+C чтобы издать сигнал, Ctrl+\\ чтобы завершить программу\n");

    for (;;) {
        pause();
    }

    return 0;
}
