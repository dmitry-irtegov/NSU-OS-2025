#include "core/error_handler.h"


error_info_t error_info = {ERROR_NONE, NULL};

void set_error(error_t type, const char* message) {
    clear_error();
    error_info.type = type;
    if (message) {
        error_info.message = strdup(message);
    } else {
        error_info.message = NULL;
    }
}

void clear_error() {
    if (error_info.message) {
        free(error_info.message);
        error_info.message = NULL;
    }
    error_info.type = ERROR_NONE;
}

int has_error() {
    return error_info.type != ERROR_NONE;
}
void print_error() {
    if (has_error()) {
        fprintf(stderr, "Error:");
        if (error_info.message) {
            fprintf(stderr, " %s\n", error_info.message);
        } else {
            fprintf(stderr, " Unknown error\n");
        }
    }
    fprintf(stderr, "\n");
}