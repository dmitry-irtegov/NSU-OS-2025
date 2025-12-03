#ifndef __IO_H
#define __IO_H

#include <sys/types.h>

int check_terminal();
int save_terminal();
int restore_terminal();
int set_foreground(pid_t pgid);

void prompt_init();
char *prompt_line(char *prev);

#endif // __IO_H
