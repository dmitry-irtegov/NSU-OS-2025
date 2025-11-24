#ifndef __IO_H
#define __IO_H

#include <termios.h>

#define RD_BUF_SIZE 4096

int setup_terminal(int fd, struct termios *old, struct termios *new);
int restore_terminal(int fd, struct termios *old);

int fdprintf(int fd, const char *format, ...);

int prompt_line(char *buffer, int len, char *is_eof);

#endif // __IO_H
