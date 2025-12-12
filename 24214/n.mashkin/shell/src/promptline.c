#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "shell.h"

int promptline(char *line, int sizline) {
    char prompt[1000];
    char *dir = getcwd(NULL, 500);
    char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) {
            home_dir = pwd->pw_dir;
        } else {
            home_dir = "/home/students/24200/n.mashkin";
        }
    }
    if (strstr(dir, home_dir) == dir) {
        dir[0] = '~';
        for (size_t home_dir_len = strlen(home_dir), i = home_dir_len; i <= strlen(dir); i++) {
            dir[i - home_dir_len + 1] = dir[i];
        }
    }
    snprintf(prompt, 999, "\033[38;2;91;194;91m{glorp}\033[0m \033[38;2;175;175;96m[%s]\033[0m $> ", dir);

    int n = 0;
    write(1, prompt, strlen(prompt));
    while (1) {
        n += read(0, (line + n), sizline-n);
        *(line+n) = '\0';
        /*
*  check to see if command line extends onto
*  next line.  If so, append next line to command line
*/

        if (*(line+n-2) == '\\' && *(line+n-1) == '\n') {
            *(line+n) = ' ';
            *(line+n-1) = ' ';
            *(line+n-2) = ' ';
            continue;   /*  read next line  */
        }
        return(n);      /* all done */
    }
}
