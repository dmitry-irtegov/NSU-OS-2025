#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"

static char *blankskip(register char *);

int parseline(char *line) {
    register char *s;
    int aflg = 0;
    int pind = 0, cind = 0, aind = 0;
    static char delim[] = " \t|&<>;\n";

    /* initialize  */
    pipelines[pind].bkgrnd = 0;
    pipelines[pind].ncmds = 0;
    pipelines[pind].cmds[cind].nargs = 0;
    pipelines[pind].infile = NULL;
    pipelines[pind].outfile = NULL;
    pipelines[pind].appfile = NULL;
    pipelines[pind].cmds[cind].cmdargs[aind] = NULL;
    s = line;

    while (*s) {        /* until line has been parsed */
        s = blankskip(s);       /*  skip white space */
        if (!*s) break; /*  done with line */
        /*  handle <, >, |, &, and ;  */
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
                if (s && isspace(*s))
                    *s++ = '\0';
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
                if (s && isspace(*s))
                    *s++ = '\0';
                break;
            case '|':
                if (pipelines[pind].cmds[cind].nargs == 0) {
                    fprintf(stderr, "Syntax error: No commands before '|' found\n");
                    return(-1);
                }
                *s++ = '\0';
                pipelines[pind].cmds[++cind].nargs = 0;
                pipelines[pind].cmds[cind].cmdargs[aind = 0] = NULL;
                break;
            case ';':
                if (pipelines[pind].ncmds == 0) {
                    fprintf(stderr, "Syntax error: No commands before ';' found\n");
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
                /*  a command argument  */
                if (pipelines[pind].cmds[cind].nargs == 0) {
                    /* next command */
                    pipelines[pind].ncmds++;
                }
                pipelines[pind].cmds[cind].cmdargs[aind++] = s;
                pipelines[pind].cmds[cind].cmdargs[aind] = NULL;
                pipelines[pind].cmds[cind].nargs++;
                s = strpbrk(s, delim);
                if (s && isspace(*s))
                    *s++ = '\0';
                break;
        }  /*  close switch  */
    }  /* close while  */
    /*  error check  */
    /*
*  The only errors that will be checked for are
*  no command on the right side of a pipe
*  no command to the left of a pipe is checked above
*/

    if (pipelines[pind].ncmds != 0 && pipelines[pind].cmds[cind].nargs == 0) {
        fprintf(stderr, "Syntax error: End of command line found, command expected\n");
        return(-1);
    }
    return pipelines[pind].ncmds == 0 ? pind : pind + 1;
}

static char* blankskip(register char *s) {
    while (isspace(*s) && *s) ++s;
    return(s);
}
