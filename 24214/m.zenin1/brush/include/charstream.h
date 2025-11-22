#pragma once

#include <stddef.h>

typedef struct charstream {
	char *arr;
	size_t size, cap;
} charstream;

charstream charstream_open();
void charstream_push(charstream *stream, char c);
char *charstream_close(charstream *stream);

