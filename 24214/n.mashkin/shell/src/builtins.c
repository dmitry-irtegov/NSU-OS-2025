#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <stdio.h>
#include <errno.h>
#include "builtins.h"
#include "shell.h"
#include "job_control.h"

#ifdef GLORP
#include <ftw.h>

int unglorpify(const char *path, const struct stat *sb, int typeflag) {
    if (typeflag == FTW_D) {
        return 0;
    }
    int i = strlen(path);
    char *to_find = "prolg.";
    for (int j = 0; j < 6; j++) {
        if (to_find[j] != path[--i]) {
            return 0;
        }
    }
    char *new_path = malloc(strlen(path) + 1);
    new_path[0] = 0;
    strcpy(new_path, path);
    new_path[i] = 0;
    rename(path, new_path);
    free(new_path);
    return 0;
}

void unglorp() {
    if (ftw(".", unglorpify, 67) == -1) {
        perror("ftw");
        exit(1);
    }
}

int glorpify(const char *path, const struct stat *sb, int typeflag) {
    if (typeflag == FTW_D) {
        return 0;
    }
    char *new_path = malloc(strlen(path) + 7);
    new_path[0] = 0;
    strcpy(new_path, path);
    strcat(new_path, ".glorp");
    rename(path, new_path);
    free(new_path);
    return 0;
}

void glorp() {
    if (ftw(".", glorpify, 67) == -1) {
        perror("ftw");
        exit(1);
    }
}
#endif

void jobs() {
    update_job_status();
    
    for (job_t *j = job_list; j; j = j->next) {
        fprintf(stderr, "[%d] %c    %s\n", j->jid, j->state, j->cmdline);
    }
}

void fg(command_t *cmd) {
    job_t *j = get_job_by_spec(cmd->cmdargs[1]);
    if (!j) {
        fprintf(stderr, "fg: job not found\n");
        return;
    }
    
    put_job_in_foreground(j);
}

void bg(command_t *cmd) {
    job_t *j = get_job_by_spec(cmd->cmdargs[1]);
    if (!j) {
        fprintf(stderr, "bg: job not found\n");
        return;
    }
    
    put_job_in_background(j);
}

void cd(command_t *cmd) {
    char *path = malloc(MAXPATH);
    
    if (cmd->cmdargs[1] == NULL) {
        cmd->cmdargs[1] = "~";
        cmd->cmdargs[2] = NULL;
    }

    if (cmd->cmdargs[1][0] == '~') {
        char *home = getenv("HOME");
        if (home == NULL) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) {
                home = pw->pw_dir;
            } else {
                fprintf(stderr, "cd: HOME not set\n");
                free(path);
                return;
            }
        }
        snprintf(path, MAXPATH - 1, "%s%s", home, cmd->cmdargs[1] + 1);
    } else if (strcmp(cmd->cmdargs[1], "-") == 0) {
        char *old = getenv("OLDPWD");
        if (old == NULL) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            free(path);
            return;
        }
        printf("%s\n", old);
        strncpy(path, old, MAXPATH - 1);
    } else {
        strncpy(path, cmd->cmdargs[1], MAXPATH);
    }
    
    char *oldpwd = getcwd(NULL, MAXPATH);
    if (oldpwd) {
        setenv("OLDPWD", oldpwd, 1);
        free(oldpwd);
    }
    
    if (chdir(path) != 0) {
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    } else {
        char *newpwd = getcwd(NULL, MAXPATH);
        if (newpwd) {
            setenv("PWD", newpwd, 1);
            free(newpwd);
        }
    }
    free(path);
}

void ls(command_t *cmd) {
    if (cmd->nargs + 1 > MAXARGS) {
        return;
    }

    char *new_arg = "--color=auto";
    for (int i = cmd->nargs + 1; i > 1; i--) {
        cmd->cmdargs[i] = cmd->cmdargs[i - 1];
    }
    cmd->cmdargs[1] = malloc(strlen(new_arg) + 1);
    strcpy(cmd->cmdargs[1], new_arg);
}
