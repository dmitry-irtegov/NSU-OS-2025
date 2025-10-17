#include <stdlib.h>
#include "parse/user_input.h"
#include "parse/pipeline.h"
#include "ptr_vec.h"

user_input user_input_construct(){
	return (user_input) {ptr_vec_construct()};
}

pipeline* user_input_allocate_new_pipeline(user_input *inp){
	pipeline *new_pipl = malloc(sizeof(pipeline));
	*new_pipl = pipeline_construct();

	ptr_vec_push(&inp->pipelines, new_pipl);
	return new_pipl;
}

void user_input_destruct(user_input *inp){
	for (size_t i = 0; i < inp->pipelines.size; i++){
		pipeline_destruct((pipeline*) inp->pipelines.arr[i]);
		free(inp->pipelines.arr[i]);
	}

	ptr_vec_destruct(&inp->pipelines);
}

