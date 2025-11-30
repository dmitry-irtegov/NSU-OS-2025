#ifndef __IO_H
#define __IO_H

#include <termios.h>

int check_terminal(int fd);

int setup_terminal(int fd, struct termios *old);
int save_terminal(int fd, struct termios *attrs);
int restore_terminal(int fd, struct termios *attrs);

int set_foreground(int fd, pid_t pgid);

int prompt_line(char *buffer, int len, char *is_eof);

#endif // __IO_H
