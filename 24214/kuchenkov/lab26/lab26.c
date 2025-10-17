#include <stdio.h>

int main() {
    FILE *pipe_in, *pipe_out;
    char line[BUFSIZ];

    pipe_in = popen("echo 'Ya normalniy\nNormalniy'", "r");
    pipe_out = popen("tr 'a-z' 'A-Z'", "w");

    while(fgets(line, BUFSIZ, pipe_in) != NULL) {
        fputs(line, pipe_out);
    }

    pclose(pipe_in);
    pclose(pipe_out);
    
    return 0;
}