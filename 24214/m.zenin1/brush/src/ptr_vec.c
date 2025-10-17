#include <stdlib.h>
#include "ptr_vec.h"

ptr_vec ptr_vec_construct(){
	return (ptr_vec) {NULL, 0, 0};
}

void ptr_vec_push(ptr_vec *vec, void *ptr){
	if (vec->cap == 0){ 
		vec->arr = malloc(sizeof(void*)); 
		vec->cap = 1; 
	} 
	else if (vec->cap == vec->size){ 
		vec->arr = realloc(vec->arr, vec->cap * 2 * sizeof(void*)); 
		vec->cap *= 2; 
	} 

	vec->arr[vec->size++] = ptr; 
}

void ptr_vec_destruct(ptr_vec *vec){
	if (vec->arr != NULL){
		free(vec->arr);
		vec->arr = NULL;
		vec->size = 0;
		vec->cap = 0;
	}
}

