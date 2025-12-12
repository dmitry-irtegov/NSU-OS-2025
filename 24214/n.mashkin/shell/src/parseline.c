#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"

char* blankskip(char *s) {
    while (isspace(*s) && *s++);
    return s;
}

int parseline(char *line) {
    char *s;
    int aflg = 0;
    int pind = 0, cind = 0, aind = 0;
    char delim[] = " \t|&<>;\n";

    pipelines[pind].bkgrnd = 0;
    pipelines[pind].ncmds = 0;
    pipelines[pind].cmds[cind].nargs = 0;
    pipelines[pind].infile = NULL;
    pipelines[pind].outfile = NULL;
    pipelines[pind].appfile = NULL;
    pipelines[pind].cmds[cind].cmdargs[aind] = NULL;

    s = line;
    while (s && *s) {
        s = blankskip(s);
        if (!*s) break;

        switch(*s) {
            case '&':
                pipelines[pind].bkgrnd = 1;
                *s++ = '\0';
                break;
            case '>':
                if (*(s + 1) == '>') {
                    aflg = 1;
                    *s++ = '\0';
                }
                *s++ = '\0';
                s = blankskip(s);
                if (!*s) {
                    fprintf(stderr, "Syntax error: No output file found\n");
                    return(-1);
                }
                if (aflg) {
                    pipelines[pind].appfile = s;
                } else {
                    pipelines[pind].outfile = s;
                }
                s = strpbrk(s, delim);
                if (s && isspace(*s)) {
                    *s++ = '\0';
                }
                break;
            case '<':
                *s++ = '\0';
                s = blankskip(s);
                if (!*s) {
                    fprintf(stderr, "Syntax error: No input file found\n");
                    return(-1);
                }
                pipelines[pind].infile = s;
                s = strpbrk(s, delim);
                if (s && isspace(*s)) {
                    *s++ = '\0';
                }
                break;
            case '|':
                if (pipelines[pind].cmds[cind].nargs == 0) {
                    fprintf(stderr, "Syntax error: No commands found before '|'\n");
                    return(-1);
                }
                *s++ = '\0';
                pipelines[pind].cmds[++cind].nargs = 0;
                pipelines[pind].cmds[cind].cmdargs[aind = 0] = NULL;
                break;
            case ';':
                if (pipelines[pind].ncmds == 0) {
                    fprintf(stderr, "Syntax error: No commands found before ';'\n");
                    return -1;
                }
                *s++ = '\0';
                pipelines[++pind].bkgrnd = 0;
                pipelines[pind].ncmds = 0;
                pipelines[pind].cmds[cind = 0].nargs = 0;
                pipelines[pind].infile = NULL;
                pipelines[pind].outfile = NULL;
                pipelines[pind].appfile = NULL;
                pipelines[pind].cmds[cind].cmdargs[aind = 0] = NULL;
                break;
            default:
                if (pipelines[pind].cmds[cind].nargs == 0) {
                    pipelines[pind].ncmds++;
                }
                pipelines[pind].cmds[cind].cmdargs[aind++] = s;
                pipelines[pind].cmds[cind].cmdargs[aind] = NULL;
                pipelines[pind].cmds[cind].nargs++;
                s = strpbrk(s, delim);
                if (s && isspace(*s)) {
                    *s++ = '\0';
                }
                break;
        }
    }

    if (pipelines[pind].ncmds != 0 && pipelines[pind].cmds[cind].nargs == 0) {
        fprintf(stderr, "Syntax error: End of command line found, command expected\n");
        return -1;
    }
    return pipelines[pind].ncmds == 0 ? pind : pind + 1;
}

