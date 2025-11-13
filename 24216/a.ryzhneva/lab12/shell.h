#define MAXARGS 256
#define MAXCMDS 50

struct command
{
    char *cmdargs[MAXARGS];
    char cmdflag;
};

#define OUTPIP 01
#define INPIP 02

extern struct termios shell_termios;
extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

int parseline(char *);
int promptline(char *, char *, int);