#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

#ifdef GLORP
void glorp();
void unglorp();
#endif
void bg(command_t *);
void fg(command_t *);
void jobs(); 
void ls(command_t *);
void cd(command_t *);

#endif
