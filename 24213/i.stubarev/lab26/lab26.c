#include <stdio.h>

int main(void) {
    const char text[] = "Hello worLD. MY Name Is ILYA123";
    
    FILE *pipe = popen("tr 'a-z' 'A-Z'", "w");
    if (!pipe) {
        perror("popen failed");
        return 1;
    }

    if (fprintf(pipe, "%s\n", text) < 0) {
        perror("write to pipe failed");
        int cl_status = pclose(pipe);
        if (cl_status == -1) {
            perror("pclose also failed after write error");
        }
        return 1;
    }

    int status = pclose(pipe);
    if (status == -1) {
        perror("pclose failed");
        return 1;
    }
    if (status != 0) {
        fprintf(stderr, "tr command exited with status %d\n", status);
        return 1;
    }

    return 0;
}
