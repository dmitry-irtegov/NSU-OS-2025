#include "execution/pipeline.h"

pipeline_t* create_pipeline(void) {
    pipeline_t* pipeline = malloc(sizeof(pipeline_t));
    if (!pipeline) {
        return NULL;
    }
    pipeline->cmds = NULL;
    pipeline->count = 0;
    pipeline->cap = 0;
    pipeline->input_file = NULL;
    pipeline->output_file = NULL;
    pipeline->append_file = NULL;
    pipeline->is_background = 0;
    return pipeline;
}

void destroy_pipeline(pipeline_t* pipeline) {
    if (pipeline) {
        for (size_t i = 0; i < pipeline->count; i++) {
            destroy_command(pipeline->cmds[i]);
        }
        free(pipeline->cmds);
        free(pipeline->input_file);
        free(pipeline->output_file);
        free(pipeline->append_file);
        free(pipeline);
    }
}

void push_pipeline_command(pipeline_t* pipeline, command_t* cmd) {
    if (pipeline->cap == 0) {
        pipeline->cap = 1;
        pipeline->cmds = malloc(pipeline->cap * sizeof(command_t*));
    } else if (pipeline->count == pipeline->cap) {
        pipeline->cap *= 2;
        pipeline->cmds = realloc(pipeline->cmds, pipeline->cap * sizeof(command_t*));
    }
    pipeline->cmds[pipeline->count++] = cmd;
}