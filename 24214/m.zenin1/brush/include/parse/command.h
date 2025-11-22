#pragma once

#include "ptr_vec.h"
#include <unistd.h>

typedef struct command {
	ptr_vec args;
} command;

command command_construct();

void command_add_arg(command *cmd, char *arg);

void command_destruct(command *cmd);

