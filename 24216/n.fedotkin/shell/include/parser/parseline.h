#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "execution/command.h"
#include "parser/task_manager.h"
#include "execution/pipeline.h"
#include "core/error_handler.h"

task_manager_t* parse_line(char* line);
size_t calculate_unescaped_length(char* start, size_t length);
char* create_unescaped_string(char* start, size_t length);
void skip_whitespaces(char** stream);
int check_to_end_of_stream(char symbol);
int check_to_whitespace(char symbol);
int check_delimeter(char symbol);
char* extract_next_arg(char** stream);
command_t* parse_command(char** stream);
int check_command_delimeter(char symbol);
int check_pipe_delimeter(char symbol);
int check_redirect(char symbol);
pipeline_t* parse_pipeline(char** stream);
int parse_redirects(char** stream, pipeline_t* pipeline);