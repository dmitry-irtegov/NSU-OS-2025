#include <stdlib.h>
#include "charstream.h"

charstream charstream_open(){
	return (charstream) {NULL, 0, 0};
}

void charstream_push(charstream *stream, char c){
	if (stream->cap == 0){ 
		stream->arr = malloc(sizeof(char)); 
		stream->cap = 1; 
	} 
	else if (stream->cap == stream->size){ 
		stream->arr = realloc(stream->arr, stream->cap * 2 * sizeof(char)); 
		stream->cap *= 2; 
	} 

	stream->arr[stream->size++] = c; 
}

char *charstream_close(charstream *stream){
	return stream->arr;
}

