#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "pipeline.h"

static char *blankskip(register char *s) {
	while (isspace(*s) && *s) {
		++s;
	}
  return(s);
}

int parse_pipeline(char *line, pipeline_t *pipeline) {
		pipeline->cmd_count = 0;
		pipeline->foreground = 1;
		pipeline->infile = 0;
		pipeline->outfile = 0;
		pipeline->appfile = 0;
	
    static char delim[] = " \t|&<>;\n";

    if (line[0] == '\n') {
        return (-1);
    }

    char aflg = 0;
    int nargs = 0;
		int rval = 0;
    for (int i = 0; i < MAXCMDS; i++) {
			pipeline->cmds[i].cmdflag = 0;
		}

    char *s = line;
    while (*s) {        
        s = blankskip(s); 
        if (!*s) {
					break; 
				}

        switch(*s) {
            case '&':	
								pipeline->foreground = 0;
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
                    fprintf(stderr, "Syntax error\n");
                    return(-1);
                }

                if (aflg) {
                	pipeline->appfile = s;
								}
                else {
                	pipeline->outfile = s;
								}
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
            case '<':
                *s++ = '\0';
                s = blankskip(s);
                if (!*s) {
                    fprintf(stderr, "Syntax error\n");
                    return(-1);
                }
                pipeline->infile = s;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
            case '|':
                if (nargs == 0) {
                    fprintf(stderr, "Syntax error\n");
                    return(-1);
                }
								pipeline->cmds[pipeline->cmd_count++].cmdflag |= OUTPIP;
								pipeline->cmds[pipeline->cmd_count].cmdflag |= INPIP;
                *s++ = '\0';
                nargs = 0;
                break;
            case ';':
                if (nargs == 0) {
                    fprintf(stderr, "Syntax error\n");
                    return(-1);
                }
                *s++ = '\0';
                pipeline->cmd_count++;
                nargs = 0;
                break;
            default:
                if (nargs == 0) {
                    rval = pipeline->cmd_count + 1;
								}
								pipeline->cmds[pipeline->cmd_count].cmdargs[nargs++] = s;
								pipeline->cmds[pipeline->cmd_count].cmdargs[nargs] = (char*) NULL;
                s = strpbrk(s, delim);
                if (isspace(*s))
                    *s++ = '\0';
                break;
        } 
    }  

    if (pipeline->cmds[pipeline->cmd_count - 1].cmdflag & OUTPIP) {
        if (nargs == 0) {
            fprintf(stderr, "Syntax error\n");
            return(-1);
        }
    }

	  if (pipeline->cmd_count == MAXCMDS) {
        fprintf(stderr, "Too many commands in pipeline.\n");
        return 1;
    }
		pipeline->cmds[pipeline->cmd_count++].cmdargs[nargs] = NULL;
    return(rval);
}
