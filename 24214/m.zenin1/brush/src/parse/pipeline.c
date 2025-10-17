#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "parse/pipeline.h"
#include "parse/command.h"
#include "ptr_vec.h"

pipeline pipeline_construct(){
	return (pipeline) {ptr_vec_construct(), true, NULL, NULL, NULL, NULL};
}

// allocates new command structure in pipeline, returns pointer to it
command* pipeline_allocate_new_command(pipeline *pipl){
	command *new_cmd = malloc(sizeof(command));
	*new_cmd = command_construct();

	ptr_vec_push(&pipl->commands, new_cmd);
	return new_cmd;
}

// set filename for in/out/app
// if filename is already set, replace it with new filename
void pipeline_set_infile(pipeline *pipl, char *file){
	if (pipl->infile){
		free(pipl->infile);
	}
	pipl->infile = file;
}

void pipeline_set_outfile(pipeline *pipl, char *file){
	if (pipl->outfile){
		free(pipl->outfile);
	}
	pipl->outfile = file;
}

void pipeline_set_appfile(pipeline *pipl, char *file){
	if (pipl->appfile){
		free(pipl->appfile);
	}
	pipl->appfile = file;
}

// Copies specified line
void pipeline_set_original_line(pipeline *pipl, char *line, size_t size){
	if (pipl->original_line){
		free(pipl->original_line);
	}

	pipl->original_line = malloc((size + 1) * sizeof(char));

	strlcpy(pipl->original_line, line, size + 1);

	for (size_t i = 0; i < size; i++){
		if (pipl->original_line[i] == '\n'){
			pipl->original_line[i] = ' ';
		}
	}
}

void pipeline_destruct(pipeline *pipl){
	for (size_t i = 0; i < pipl->commands.size; i++){
		command_destruct((command*) pipl->commands.arr[i]);
		free(pipl->commands.arr[i]);
	}
	ptr_vec_destruct(&pipl->commands);

	if (pipl->original_line){
		free(pipl->original_line);
	}
	if (pipl->infile){
		free(pipl->infile);
	}
	if (pipl->outfile){
		free(pipl->outfile);
	}
	if (pipl->appfile){
		free(pipl->appfile);
	}
}

