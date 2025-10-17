#pragma once

#include <stdbool.h>
#include "parse/command.h"
#include "ptr_vec.h"

// commands - array of separate commands
// foreground - if false, whole pipeline must be executed in background, otherwise, pipeline must be executed in foreground
// original_line - line, which was parsed into this pipeline
// infile - if not NULL - pipelines' stdin must be this file instead of terminal stdin
// outfile - if not NULL - pipelines' stdout must be this file instead of terminal stdout, also this file must be opened for rewriting
// appfile - if not NULL - pipelines' stdout must be this file instead of terminal stdout, also this file must be opened for appending
typedef struct pipeline {
	ptr_vec commands;
	bool foreground;
	char *original_line;
	char *infile;
	char *outfile;
	char *appfile;
} pipeline;

pipeline pipeline_construct();

// allocates new command structure in pipeline, returns pointer to it
command* pipeline_allocate_new_command(pipeline *pipl);

// set filename for in/out/app
// if filename is already set, replace it with new filename
void pipeline_set_infile(pipeline *pipl, char *file);
void pipeline_set_outfile(pipeline *pipl, char *file);
void pipeline_set_appfile(pipeline *pipl, char *file);

void pipeline_set_original_line(pipeline *pipl, char *line, size_t size);

void pipeline_destruct(pipeline *pipl);

