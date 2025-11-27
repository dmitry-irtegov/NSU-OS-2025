#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include "core/shell.h"
#include "io/readline.h"
#include "parser/parseline.h"
#include "execution/launcher.h"
#include "scheduler/task_list.h"
#include "core/error_handler.h"

void shell_init(void);

pid_t shell_get_pgid(void);
struct termios* shell_get_tmodes(void);
task_list_t* shell_get_tasks(void);

void shell_run(void);

void shell_check_background_tasks(void);

int shell_execute_builtin(char* cmd, char** argv, int argc);