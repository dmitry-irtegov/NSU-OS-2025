#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "ptr_vec.h"
#include "parse/pipeline.h"

typedef struct user_input {
	ptr_vec pipelines;
} user_input;

user_input user_input_construct();

// Allocates new pipeline structure in user_input, returns pointer to it
pipeline* user_input_allocate_new_pipeline(user_input *inp);

void user_input_destruct(user_input *inp);

