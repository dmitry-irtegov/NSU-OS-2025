#include "io/readline.h"

#define INPUT_BUFFER_CHUNK_SIZE 128

static void display_prompt(int is_multiline);
static reader_t* create_reader();
static void destroy_reader(reader_t *reader);
static int reader_ensure_capacity(reader_t *reader);
static void reader_update_state(reader_t *reader, size_t scan_start);

char* read_line() {
    reader_t* reader = create_reader();
    if (!reader) {
        perror("Failed to create reader");
        return NULL;
    }
    reader->buffer[0] = '\0';

    while(1) {
        display_prompt(reader->is_multiline);

        if (!reader_ensure_capacity(reader)) {
            destroy_reader(reader);
            return NULL;
        }
        size_t scan_start = reader->chars_read;
        if (fgets(reader->buffer + reader->chars_read, reader->buffer_size - reader->chars_read, stdin) == NULL) {
            if (errno || reader->in_quotes) {
                perror("Error reading input");
                destroy_reader(reader);
                return NULL;
            } else if (reader->chars_read == 0) {
                fprintf(stdout,"\n");
                destroy_reader(reader);
                return NULL;
            } else {
                break;
            }
        }
        reader->chars_read = strlen(reader->buffer);
        reader_update_state(reader, scan_start);

        if (reader->buffer[reader->chars_read - 1] != '\n') {
            continue;
        }

        if (!reader->in_quotes && reader->chars_read >= 2 && reader->buffer[reader->chars_read - 2] == '\\') {
            reader->buffer[reader->chars_read - 2] = '\0';
            reader->chars_read -= 2;
            reader->is_multiline = 1;
            continue;
        }

        if (reader->in_quotes) {
            reader->is_multiline = 1;
            continue;
        }

        break;
    }
    char *result = strdup(reader->buffer);
    destroy_reader(reader);
    return result;
}



static void display_prompt(int is_multiline) {
    if (is_multiline) {
        printf("> ");
    } else {
        printf("shell$ ");
    }
    fflush(stdout);
}


static reader_t* create_reader() {
    reader_t* reader = malloc(sizeof(reader_t));
    if (!reader) {
        return NULL;
    }
    reader->buffer = malloc(INPUT_BUFFER_CHUNK_SIZE * sizeof(char));
    if (!reader->buffer) {
        free(reader);
        return NULL;
    }
    reader->buffer_size = INPUT_BUFFER_CHUNK_SIZE;
    reader->chars_read = 0;
    reader->in_quotes = 0;
    reader->is_multiline = 0;

    return reader;
}

static void destroy_reader(reader_t *reader) {
    if (reader) {
        free(reader->buffer);
        free(reader);
    }
}

static void reader_update_state(reader_t *reader, size_t scan_start) {
    for (size_t i = scan_start; i < reader->chars_read; ++i) {
        if (reader->buffer[i] == '"') {
            if (i > 0 && reader->buffer[i-1] == '\\') {
                continue;
            }
            reader->in_quotes = !reader->in_quotes;
        }
    }
}

static int reader_ensure_capacity(reader_t *reader) {
    if (reader->chars_read + INPUT_BUFFER_CHUNK_SIZE > reader->buffer_size) {
        size_t new_size = reader->buffer_size + INPUT_BUFFER_CHUNK_SIZE;
        char *tmp = realloc(reader->buffer, new_size);
        if (!tmp) {
            perror("Failed to reallocate input buffer");
            return 0;
        }
        reader->buffer = tmp;
        reader->buffer_size = new_size;
    }
    return 1;
}