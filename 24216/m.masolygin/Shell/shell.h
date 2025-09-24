#include <unistd.h>

#define MAXARGS 256
#define MAXCMDS 50

struct command
{
    char *cmdargs[MAXARGS];
    char cmdflag;
};

/*  cmdflag's  */
#define OUTPIP 01
#define INPIP 02

extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

int parseline(char *);
int promptline(char *, char *, int);
void handler_child(int);
void cleanup_zombies(void);