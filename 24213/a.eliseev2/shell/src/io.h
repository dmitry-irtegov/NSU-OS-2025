#ifndef __IO_H
#define __IO_H

#include <termios.h>

int check_terminal(int fd);

int save_terminal(int fd, struct termios *attrs);
int restore_terminal(int fd, struct termios *attrs);

int set_foreground(int fd, pid_t pgid);

void prompt_init();
char *prompt_line(char *prev);

#endif // __IO_H
