#include "io.h"
#include "parse.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {

    /* PLACE SIGNAL CODE HERE */

    char is_eof = 0, is_error = 0;
    char line[RD_BUF_SIZE];
    
    struct termios new_attr;
    struct termios old_attr;
    if (setup_terminal(0, &old_attr, &new_attr)) {
        is_error = 1;
    }

    while (!is_error && !is_eof && prompt_line(line, sizeof(line), &is_eof)) {
        static pipeline_t pipeline;

        char *ptr = line;
        while (!is_error && parse_pipeline(&ptr, &pipeline)) {
        }
    }
    if (restore_terminal(0, &old_attr)) {
        is_error = 1;
    }
    return !is_eof || is_error;
}

/* PLACE SIGNAL CODE HERE */
