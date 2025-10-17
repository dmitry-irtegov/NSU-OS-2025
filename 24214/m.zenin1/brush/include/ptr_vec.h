#pragma once

#include <stddef.h>

typedef struct ptr_vec {
	void **arr;
	size_t size, cap;
} ptr_vec;

ptr_vec ptr_vec_construct();
void ptr_vec_push(ptr_vec *vec, void *ptr);
void ptr_vec_destruct(ptr_vec *vec);

