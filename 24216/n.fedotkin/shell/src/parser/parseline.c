#include "parser/parseline.h"

task_manager_t* parse_line(char* line) {
    char* stream = line;
    task_manager_t* task_manager = create_task_manager();
    if (!task_manager) {
        set_error(ERROR_MEMORY_ALLOCATION, "Failed to create task manager");
        return NULL;
    }

    while (!check_to_end_of_stream(*stream)) {
        skip_whitespaces(&stream);
        if (check_to_end_of_stream(*stream)) {
            break;
        }

        pipeline_t* pipeline = parse_pipeline(&stream);
        if (!pipeline) {
            destroy_pipeline(pipeline);
            print_error();
            clear_error();
            return NULL;
        }

        add_pipeline_task_manager(task_manager, pipeline);

        skip_whitespaces(&stream);

        if (check_pipe_delimeter(*stream)) {
            stream++;
            continue;
        } else {
            break;
        }
    }

    return task_manager;
}

pipeline_t* parse_pipeline(char** stream) {
    pipeline_t* pipeline = create_pipeline();
    if (!pipeline) {
        set_error(ERROR_MEMORY_ALLOCATION, "Failed to create pipeline");
        return NULL;
    }

    while (1) {
        skip_whitespaces(stream);
        if (check_to_end_of_stream(**stream) || check_delimeter(**stream)) {
            break;
        }

        if (check_redirect(**stream)) {
            if (!parse_redirects(stream, pipeline)) {
                destroy_pipeline(pipeline);
                return NULL;
            }
            continue;
        }
        
        command_t* cmd = parse_command(stream);
        
        if (cmd) {
            push_pipeline_command(pipeline, cmd);
        } else {
            destroy_pipeline(pipeline);
            set_error(ERROR_EMPTY_COMMAND, "Failed to parse command in pipeline");
            return NULL;
        }

        skip_whitespaces(stream);

        if (check_command_delimeter(**stream)) {
            (*stream)++;
            continue;
        } else if (check_redirect(**stream)) {
            continue;
        } else {
            break;
        }
    }

    skip_whitespaces(stream);
    if (**stream == '&') {
        pipeline->is_background = 1;
    }
    if (pipeline->count == 0) {
        destroy_pipeline(pipeline);
        set_error(ERROR_EMPTY_PIPELINE, "Pipeline is empty");
        return NULL;
    }

    return pipeline;
}

command_t* parse_command(char** stream) {
    command_t* cmd = create_command();
    if (!cmd) {
        set_error(ERROR_MEMORY_ALLOCATION, "Failed to create command");
        return NULL;
    }
    char* new_arg = NULL;

    while (1) {
        skip_whitespaces(stream);
        if (check_to_end_of_stream(**stream) || check_delimeter(**stream)
         || check_redirect(**stream) || check_command_delimeter(**stream)) {
            break;
        }

        new_arg = extract_next_arg(stream);
        if (new_arg) {
            push_command_arg(cmd, new_arg);
        }
    }

    if (cmd->argc == 0) {
        destroy_command(cmd);
        set_error(ERROR_EMPTY_COMMAND, "Command has no arguments");
        return NULL;
    }

    return cmd;
}

char* extract_next_arg(char** stream) {
    char* start = *stream;
    int is_escaped = 0;
    char quote_type = 0;

    while (!check_to_end_of_stream(**stream)) {
        char cur = **stream;

        if (cur == '\\' && !is_escaped) {
            is_escaped = 1;
        } else if ((cur == '"' || cur == '\'') && !is_escaped) {
            if (quote_type == 0) {
                quote_type = cur;
            } else if (quote_type == cur) {
                quote_type = 0;
            }
        } else if ((check_to_whitespace(cur) || check_delimeter(cur)
         || check_redirect(cur) || check_command_delimeter(cur)) && quote_type == 0 && !is_escaped) {
            break;
        } else {
            is_escaped = 0;
        }
        (*stream)++;
    }

    if (quote_type != 0) {
        set_error(ERROR_UNMATCHED_QUOTES, "Unmatched quotes found");
        return NULL;
    }

    return create_unescaped_string(start, *stream - start);
}

int parse_redirects(char** stream, pipeline_t* pipeline) {
    char redirect_type = **stream;
    (*stream)++;

    if (redirect_type == '>' && **stream == '>') {
        (*stream)++;
        skip_whitespaces(stream);
        
        if (pipeline->output_file || pipeline->append_file) {
            set_error(ERROR_INVALID_REDIRECTION, "Multiple output redirections in same pipeline");
            return 0;
        }
        
        char* filename = extract_next_arg(stream);
        if (!filename) {
            set_error(ERROR_INVALID_REDIRECTION, "Failed to parse append redirect filename");
            return 0;
        }
        pipeline->append_file = filename;
    } else {
        skip_whitespaces(stream);

        char* filename = extract_next_arg(stream);
        if (!filename) {
            set_error(ERROR_INVALID_REDIRECTION, "Failed to parse redirect filename");
            return 0;
        }

        if (redirect_type == '<') {
            if (pipeline->input_file) {
                free(filename);
                set_error(ERROR_INVALID_REDIRECTION, "Multiple input redirections in same pipeline");
                return 0;
            }
            pipeline->input_file = filename;
        } else if (redirect_type == '>') {
            if (pipeline->output_file || pipeline->append_file) {
                free(filename);
                set_error(ERROR_INVALID_REDIRECTION, "Multiple output redirections in same pipeline");
                return 0;
            }
            pipeline->output_file = filename;
        }
    }

    return 1;
}

int check_command_delimeter(char symbol) {
    return symbol == '|';
}
int check_pipe_delimeter(char symbol) {
    return symbol == ';' || symbol == '&';
}

int check_redirect(char symbol) {
    return symbol == '<' || symbol == '>';
}

int check_to_end_of_stream(char symbol) {
    return symbol == '\0';
}

int check_to_whitespace(char symbol) {
    return isblank(symbol) || symbol == '\n';
}

void skip_whitespaces(char** stream) {
    while (check_to_whitespace(**stream) && !check_to_end_of_stream(**stream)) {
        (*stream)++;
    }
}

char* create_unescaped_string(char* start, size_t length) {
    size_t final_length = calculate_unescaped_length(start, length);
    if (final_length == 0) {
        return NULL;
    }

    char* result_str = malloc((final_length + 1) * sizeof(char));
    if (!result_str) {
        set_error(ERROR_MEMORY_ALLOCATION, "Failed to allocate string");
        return NULL;
    }

    size_t j = 0;
    int is_escaped = 0;
    char quote_type = 0;
    
    for (size_t i = 0; i < length; i++) {
        char cur = start[i];

        if ((cur == '"' || cur == '\'') && !is_escaped) {
            if (quote_type == 0) {
                quote_type = cur;
                continue;
            } else if (quote_type == cur) {
                quote_type = 0;
                continue;
            }
        }

        if (cur == '\\' && !is_escaped && quote_type != '\'') {
            is_escaped = 1;
            continue;
        }
        
        result_str[j++] = cur;
        is_escaped = 0;
    }
    
    result_str[j] = '\0';
    return result_str;
}

size_t calculate_unescaped_length(char* start, size_t length) {
    size_t final_length = 0;
    int is_escaped = 0;
    char quote_type = 0;

    for (size_t i = 0; i < length; i++) {
        char cur = start[i];

        if ((cur == '"' || cur == '\'') && !is_escaped) {
            if (quote_type == 0) {
                quote_type = cur;
                continue;
            } else if (quote_type == cur) {
                quote_type = 0;
                continue;
            }
        }

        if (cur == '\\' && !is_escaped && quote_type != '\'') {
            is_escaped = 1;
            continue;
        }
        
        final_length++;
        is_escaped = 0;
    }
    
    return final_length;
}

int check_delimeter(char symbol) {
    return check_command_delimeter(symbol) || check_pipe_delimeter(symbol);
}