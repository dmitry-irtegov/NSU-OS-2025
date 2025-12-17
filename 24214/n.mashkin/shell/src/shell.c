#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include "builtins.h"
#include "shell.h"
#include "job_control.h"

pipeline_t pipelines[MAXPPLINES];
pid_t foreground_pgid = 0;
struct termios shell_terminal;
pid_t shell_pgid;
int terminal_fd = -1;

void sigint_handler(int sig) {
    printf("\n");
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGINT);
    }
}

void sigquit_handler(int sig) {
    printf("\n");
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGQUIT);
    }
}

void sigtstp_handler(int sig) {
    printf("\n");
    if (foreground_pgid > 0) {
        kill(-foreground_pgid, SIGTSTP);
    }
}

int builtin_cmd(command_t *cmd) {
    if (!cmd->cmdargs[0]) return 0;
    
    if (strcmp(cmd->cmdargs[0], "fg") == 0) {
        fg(cmd);
        return 1;
    } else if (strcmp(cmd->cmdargs[0], "bg") == 0) {
        bg(cmd);
        return 1;
    } else if (strcmp(cmd->cmdargs[0], "jobs") == 0) {
        jobs();
        return 1;
    } else if (strcmp(cmd->cmdargs[0], "cd") == 0) {
        cd(cmd);
        return 1;
    } else if (strcmp(cmd->cmdargs[0], "ls") == 0) {
        ls(cmd);
        return 0;
#ifdef GLORP
    } else if (strcmp(cmd->cmdargs[0], "glorp") == 0) {
        glorp();
        return 1;
    } else if (strcmp(cmd->cmdargs[0], "unglorp") == 0) {
        unglorp();
        return 1;
#endif
    } else if (strcmp(cmd->cmdargs[0], "exit") == 0) {
        exit(0);
    }
    
    return 0;
}

void launch_pipeline(pipeline_t ppline, char *cmdline) {
    int pipefds[2];
    int prev_pipe_read = -1;
    pid_t pid;
    pid_t pids[MAXCMDS];
    pid_t pipeline_pgid = 0;
    
    char *cmdline_copy = strdup(cmdline);
    
    for (int i = 0; i < ppline.ncmds; i++) {
        if (builtin_cmd(&ppline.cmds[i])) {
            continue;
        }
    
        #ifdef DEBUG
        printf("Command %d: ", i);
        for (int j = 0; ppline.cmds[i].cmdargs[j]; j++) {
            printf("%s ", ppline.cmds[i].cmdargs[j]);
        }
        printf("\nbkgrnd = %d\n", ppline.bkgrnd);
        #endif

        int pipe_created = 0;
        if (i < ppline.ncmds - 1) {
            if (pipe(pipefds) < 0) {
                perror("pipe");
                exit(-1);
            }
            pipe_created = 1;
        }
        
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(-1);
        } else if (pid == 0) {
            if (i == 0) {
                setpgid(0, 0);
                pipeline_pgid = getpid();
            } else {
                setpgid(0, pipeline_pgid);
            }
            
            if (ppline.bkgrnd) {
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
            }
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            
            if (!ppline.bkgrnd) {
                tcsetpgrp(terminal_fd, pipeline_pgid);
            }
            
            if (i == 0 && ppline.infile != NULL) {
                int fd_in = open(ppline.infile, O_RDONLY);
                if (fd_in < 0) {
                    perror(ppline.infile);
                    exit(-1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            } else if (prev_pipe_read != -1) {
                dup2(prev_pipe_read, STDIN_FILENO);
                close(prev_pipe_read);
            }
            
            if (i == ppline.ncmds - 1) {
                if (ppline.outfile != NULL) {
                    int fd_out = open(ppline.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) {
                        perror(ppline.outfile);
                        exit(-1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                } else if (ppline.appfile != NULL) {
                    int fd_out = open(ppline.appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd_out < 0) {
                        perror(ppline.appfile);
                        exit(-1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }
            } else {
                dup2(pipefds[1], STDOUT_FILENO);
            }
            
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            if (pipe_created) {
                close(pipefds[0]);
                close(pipefds[1]);
            }

            execvp(ppline.cmds[i].cmdargs[0], ppline.cmds[i].cmdargs);
            perror(ppline.cmds[i].cmdargs[0]);
            exit(-1);
        } else {
            pids[i] = pid;

            if (i == 0) {
                pipeline_pgid = pid;
                setpgid(pid, pid);
            } else {
                setpgid(pid, pipeline_pgid);
            }

            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            
            if (pipe_created) {
                close(pipefds[1]);
                prev_pipe_read = pipefds[0];
            }
        }
    }
    
    if (prev_pipe_read != -1) {
        close(prev_pipe_read);
    }
    
    if (pipeline_pgid > 0) {
        add_job(pipeline_pgid, ppline.bkgrnd, ppline.ncmds, pids, cmdline_copy);
    }

    #ifdef DEBUG
    printf("pgid: %d\n", pipeline_pgid);
    #endif
    
    job_t *j;
    if (!ppline.bkgrnd) {
        foreground_pgid = pipeline_pgid;
        
        if ((j = find_job_by_pgid(pipeline_pgid))) {
            wait_for_job(j);
        }
        
        tcsetpgrp(terminal_fd, shell_pgid);
        foreground_pgid = 0;
    } else {
        if ((j = find_job_by_pgid(pipeline_pgid))) {
            printf("[%d] %d\n", j->jid, pipeline_pgid);
        }
    }
}

int main() {
    shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    terminal_fd = STDIN_FILENO;
    tcgetattr(terminal_fd, &shell_terminal);
    tcsetpgrp(terminal_fd, shell_pgid);
    
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);
    sa.sa_handler = sigtstp_handler;
    sigaction(SIGTSTP, &sa, NULL);
    sa.sa_handler = sigquit_handler;
    sigaction(SIGQUIT, &sa, NULL);
    
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    int i = 0;
    int npipelines = 0;
    char line[1024];
    while (i < npipelines || promptline(line, sizeof(line)) > 0) {
        update_job_status();
        
        if (i < npipelines) {
            #ifdef DEBUG
            printf("PIPELINE %d:\n", i);
            #endif
            launch_pipeline(pipelines[i++], line);
            usleep(10000);
            continue;
        } else if ((npipelines = parseline(line)) <= 0) {
            continue;
        }

        i = 0;
    }
    
    return 0;
}
