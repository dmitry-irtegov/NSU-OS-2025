#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "shell.h"
static char *blankskip(register char *);
int parseline(char *line)
{
    int nargs, ncmds;
    register char *s; 
    char aflg = 0; 
    int rval;
    register int i;
    static char delim[] = " \t|&<>;\n";
    bkgrnd = nargs = ncmds = rval = 0;
    s = line;
    infile = outfile = appfile = (char *) NULL;
    cmds[0].cmdargs[0] = (char *) NULL;

    for (i = 0; i < MAXCMDS; i++) {
        cmds[i].cmdflag = 0;
        cmds[i].cmdargs[0] = NULL;
    }

    while (*s) {
        s = blankskip(s);
        if (!*s) break;
        switch(*s) {
            case '&':
                ++bkgrnd;
                *s++ = '\0';
                break;
            case '>':
                if (*(s+1) == '>') {
                    ++aflg;
                    *s++ = '\0';
                }
                *s++ = '\0';
                s = blankskip(s);
                if (!*s) {
                    fprintf(stderr, "syntax error\n");
                    return(-1);
                }
                if (aflg) {
                    appfile = s;
                }
                else 
                    outfile = s;
                
                s = strpbrk(s, delim);

                if (s && isspace(*s)) {
                    *s++ = '\0';
                } else if (s) {
                    *s = '\0';
                }
                break;
            
            case '<':
                *s++ = '\0';
                s = blankskip(s);
                if(!*s) {
                    fprintf(stderr, "syntax error\n");
                    return(-1);
                }
                infile = s;
                s = strpbrk(s, delim);
                
                if (isspace(*s)) {
                    *s++ = '\0';
                } else if (s) {
                    *s = '\0';
                }
                break;
            case '|':
                if (nargs == 0) {
                    fprintf(stderr, "syntax error\n");
                    return(-1);
                }
                cmds[ncmds++].cmdflag |= OUTPIP;
                cmds[ncmds].cmdflag |= INPIP;
                *s++ = '\0';
                nargs = 0;
                break;
            case ';':
                *s++ = '\0';
                ++ncmds;
                nargs = 0;
                break;
            case '\"':
                cmds[ncmds].cmdargs[nargs++] = ++s;
                s = strpbrk(s, "\"");
                if (s == NULL) {
                    fprintf(stderr, "syntax error\n");
                    return -1;
                }
                *s++ = '\0';
                break;
            default:
                if (nargs >= MAXARGS - 1) { 
                    fprintf(stderr, "too many arguments (>%d)\n", MAXARGS - 1);
                    return(-1);
                }

                if (nargs == 0) {
                    if (ncmds >= MAXCMDS - 1) {
                        fprintf(stderr, "too many commands (>%d)\n", MAXCMDS - 1);
                        return(-1);
                    }
                    rval = ncmds+1;
                }

                cmds[ncmds].cmdargs[nargs++] = s;
                cmds[ncmds].cmdargs[nargs] = (char *) NULL;

                register char *arg_end = s;
                while (*arg_end && !strchr(delim, *arg_end)) {
                    if (*arg_end == '\\') {
                        arg_end++;
                        if (*arg_end == '\0') {
                            fprintf(stderr, "syntax error\n");
                            return -1;
                        }
                    }
                    arg_end++;
                }

                s = arg_end;
                
                if (*s) {
                    *s++ = '\0';
                }
                
                break;
        }
    }

    if (ncmds > 0 && cmds[ncmds-1].cmdflag & OUTPIP) {
        if (nargs == 0) {
            fprintf(stderr, "syntax error\n");
            return(-1);
        }
    }
    return(rval);
}

static char *blankskip(register char *s){
    while (isspace(*s) && *s) ++s;
        return(s);
}