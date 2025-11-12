#include <unistd.h>

#define MAXARGS 256
#define MAXCMDS 50

struct command {
    char* cmdargs[MAXARGS];
    char cmdflag;
};

/*  cmdflag's  */
#define OUTPIP 01
#define INPIP 02

extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

/* parseline.c */
int parseline(char*);

/* promptline.c */
int promptline(char*, char*, int);

/* utils.c */
void handler_child(int);
void file_operation(char*, int);
void cleanup_zombies(void);

/* signal.c */
void ignore_signals();
void activate_signals();