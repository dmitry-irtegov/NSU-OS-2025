#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "shell.h"

int promptline(char *line, int sizline) {
    char prompt[MAXPROMPT];
    char *dir = getcwd(NULL, MAXPATH);
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) {
            home_dir = pwd->pw_dir;
        } else {
            fprintf(stderr, "No HOME directory\n");
            exit(-1);
        }
    }
    if (strstr(dir, home_dir) == dir) {
        char *tmp = malloc(MAXPATH);
        tmp[0] = 0;
        strcpy(tmp, dir + strlen(home_dir));
        snprintf(dir, MAXPATH - 1, "~%s", tmp);
        free(tmp);
    }
    char *glorp_color = "\033[38;2;91;194;91m";
    char *dir_color = "\033[38;2;175;175;96m";
    char *clear_color = "\033[0m";
    snprintf(prompt, MAXPROMPT - 1,
             "%s{glorp}%s %s[%s]%s $> ",
             glorp_color, clear_color, dir_color, dir, clear_color);

    int n = 0;
    write(1, prompt, strlen(prompt));
    while (1) {
        n += read(0, line + n, sizline - n);
        *(line + n) = '\0';

        if (*(line + n - 2) == '\\' && *(line + n - 1) == '\n') {
            *(line + n) = ' ';
            *(line + n - 1) = ' ';
            *(line + n - 2) = ' ';
            continue;
        }
        return n;
    }
}
