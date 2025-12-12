#include <stdlib.h>
#include <stddef.h>
#include "parse/command.h"
#include "ptr_vec.h"

command command_construct(){
	return (command) {ptr_vec_construct()};
}

void command_add_arg(command *cmd, char *arg){

	ptr_vec_push(&cmd->args, arg);
}

void command_destruct(command *cmd){
	for (size_t i = 0; i < cmd->args.size; i++){
		free(cmd->args.arr[i]);
	}

	ptr_vec_destruct(&cmd->args);
}

