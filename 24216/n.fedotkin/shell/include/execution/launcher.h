#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include "scheduler/task.h"
#include "execution/process.h"
#include "core/error_handler.h"
#include "core/shell.h"

int launch_task(task_t* task);
void put_task_in_fg(task_t* task, int cont);
void put_task_in_bg(task_t* task, int cont);
void wait_for_task(task_t* task);
void close_pipes(int *pipes, int count);