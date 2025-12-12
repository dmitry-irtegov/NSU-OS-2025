#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <wait.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include "shell.h"
#include "job_control.h"

pipeline_t pipelines[MAXPPLINES];
pid_t foreground_pgid = 0;
struct termios shell_terminal;
pid_t shell_pgid;           /* PGID самого shell */
int terminal_fd = -1;       /* Файловый дескриптор терминала */

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

void sigchld_handler(int sig) {
    //update_job_status();
}

#ifdef GLORP
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

void cd(char **args) {
    char *path;
    
    // 1. cd без аргументов -> домашняя директория
    if (args[1] == NULL) {
        args[1] = "~";
        args[2] = NULL;
    }
    // 2. cd ~ -> домашняя директория
    if (strcmp(args[1], "~") == 0) {
        path = getenv("HOME");
        if (path == NULL) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) {
                path = pw->pw_dir;
            } else {
                fprintf(stderr, "cd: HOME not set\n");
                return;
            }
        }
    }
    // 3. cd - -> предыдущая директория
    else if (strcmp(args[1], "-") == 0) {
        path = getenv("OLDPWD");
        if (path == NULL) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        printf("%s\n", path);  // bash выводит путь при cd -
    }
    // 4. cd ~username -> домашняя директория пользователя
    else if (args[1][0] == '~') {
        char *username = args[1] + 1;  // пропускаем ~
        if (username[0] == '\0') {
            // Просто ~
            path = getenv("HOME");
        } else {
            struct passwd *pw = getpwnam(username);
            if (pw == NULL) {
                fprintf(stderr, "cd: no such user: %s\n", username);
                return;
            }
            path = pw->pw_dir;
        }
    }
    // 5. Обычный путь
    else {
        path = args[1];
    }
    
    // Сохраняем текущую директорию в OLDPWD перед сменой
    char *oldpwd = getcwd(NULL, 0);
    if (oldpwd) {
        setenv("OLDPWD", oldpwd, 1);
        free(oldpwd);
    }
    
    // Меняем директорию
    if (chdir(path) != 0) {
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    } else {
        // Обновляем PWD после успешного перехода
        char *newpwd = getcwd(NULL, 0);
        if (newpwd) {
            setenv("PWD", newpwd, 1);
            free(newpwd);
        }
    }
}

void ls(char **args) {
    int len = 0;
    for (; args[len]; len++);
    if (len + 1 > MAXARGS) {
        return;
    }

    char *new_arg = "--color=auto";
    args[len] = malloc(strlen(new_arg) + 1);
    strcpy(args[len], new_arg);
    args[len + 1] = NULL;
}

/* Встроенные команды */
int builtin_cmd(char **args) {
    if (!args[0]) return 0;
    
    if (strcmp(args[0], "fg") == 0) {
        do_job_fg(args);
        return 1;
    } else if (strcmp(args[0], "bg") == 0) {
        do_job_bg(args);
        return 1;
    } else if (strcmp(args[0], "jobs") == 0) {
        list_jobs();
        return 1;
    } else if (strcmp(args[0], "cd") == 0) {
        cd(args);
        return 1;
    } else if (strcmp(args[0], "ls") == 0) {
        ls(args);
        return 0;
#ifdef GLORP
    } else if (strcmp(args[0], "glorp") == 0) {
        glorp();
        return 1;
    } else if (strcmp(args[0], "unglorp") == 0) {
        unglorp();
        return 1;
#endif
    } else if (strcmp(args[0], "exit") == 0) {
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
    
    /* Копируем командную строку для хранения в задании */
    char *cmdline_copy = strdup(cmdline);
    
    /* Создаем все процессы конвейера */
    for (int i = 0; i < ppline.ncmds; i++) {
        if (builtin_cmd(ppline.cmds[i].cmdargs)) {
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
        
        /* Создаем pipe для всех команд, кроме последней */
        if (i < ppline.ncmds - 1) {
            if (pipe(pipefds) < 0) {
                perror("pipe");
                exit(1);
            }
            pipe_created = 1;
        }
        
        /* Создаем процесс */
        pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* Дочерний процесс */
            
            /* Устанавливаем группу процессов */
            if (i == 0) {
                /* Первый процесс становится лидером группы */
                setpgid(0, 0);
                pipeline_pgid = getpid();
            } else {
                /* Остальные процессы присоединяются к группе */
                setpgid(0, pipeline_pgid);
            }
            
            /* Для background процессов: блокируем доступ к терминалу */
            if (ppline.bkgrnd) {
                /* Если процесс попытается читать/писать в терминал, он получит SIGTTIN/SIGTTOU */
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
            }
            
            /* Восстанавливаем обработку сигналов */
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            
            /* Если это foreground процесс, даем ему управление терминалом */
            if (!ppline.bkgrnd) {
                /* Только foreground процессы получают управление терминалом */
                tcsetpgrp(terminal_fd, pipeline_pgid);
            } else {
                /* Background процессы НЕ получают управление терминалом */
                /* Они получат SIGTTOU если попытаются писать в терминал */
            }
            
            /* Обработка перенаправления ввода для первой команды */
            if (i == 0 && ppline.infile != NULL) {
                int fd_in = open(ppline.infile, O_RDONLY);
                if (fd_in < 0) {
                    perror(ppline.infile);
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            } else if (prev_pipe_read != -1) {
                /* Перенаправляем ввод из предыдущего pipe */
                dup2(prev_pipe_read, STDIN_FILENO);
                close(prev_pipe_read);
            }
            
            /* Обработка перенаправления вывода для последней команды */
            if (i == ppline.ncmds - 1) {
                if (ppline.outfile != NULL) {
                    int fd_out = open(ppline.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) {
                        perror(ppline.outfile);
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                } else if (ppline.appfile != NULL) {
                    int fd_out = open(ppline.appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd_out < 0) {
                        perror(ppline.appfile);
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }
            } else {
                /* Перенаправляем вывод в текущий pipe */
                dup2(pipefds[1], STDOUT_FILENO);
            }
            
            /* Закрываем все файловые дескрипторы pipe */
            if (prev_pipe_read != -1) close(prev_pipe_read);
            if (pipe_created) {
                close(pipefds[0]);
                close(pipefds[1]);
            }

            /* Выполняем команду */
            execvp(ppline.cmds[i].cmdargs[0], ppline.cmds[i].cmdargs);
            perror(ppline.cmds[i].cmdargs[0]);
            exit(1);
        } else {
            /* Родительский процесс */
            pids[i] = pid;

            if (i == 0) {
                pipeline_pgid = pid;
                setpgid(pid, pid);
            } else {
                setpgid(pid, pipeline_pgid);
            }

            /* Закрываем ненужные файловые дескрипторы pipe */
            if (prev_pipe_read != -1) {
                close(prev_pipe_read);
            }
            
            if (pipe_created) {
                close(pipefds[1]); /* Закрываем запись в pipe */
                prev_pipe_read = pipefds[0]; /* Сохраняем чтение для следующей команды */
            }
        }
    }
    
    /* Закрываем последний файловый дескриптор pipe в родителе */
    if (prev_pipe_read != -1) {
        close(prev_pipe_read);
    }
    
    /* Добавляем задание в список */
    if (pipeline_pgid > 0) {
        add_job(pipeline_pgid, ppline.ncmds, pids, cmdline_copy);
    }

    #ifdef DEBUG
    printf("pgid: %d\n", pipeline_pgid);
    #endif
    
    if (!ppline.bkgrnd) {
        /* Foreground конвейер */
        foreground_pgid = pipeline_pgid;
        
        /* Ждем завершения всех процессов конвейера */
        struct job *j = find_job_by_pgid(pipeline_pgid);
        if (j) {
            wait_for_job(j);
        }
        
        /* Возвращаем управление терминалом shell */
        tcsetpgrp(terminal_fd, shell_pgid);
        foreground_pgid = 0;
    } else {
        /* Background конвейер - НЕ даем управление терминалом */
        /* Просто выводим информацию и продолжаем */
        struct job *j = find_job_by_pgid(pipeline_pgid);
        if (j) {
            printf("[%d] %d\n", j->jid, pipeline_pgid);
        }
    }
}

int main() {
    struct sigaction sa;
    
    /* Инициализация shell */
    shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    terminal_fd = STDIN_FILENO;
    tcgetattr(terminal_fd, &shell_terminal);
    
    /* Забираем управление терминалом */
    tcsetpgrp(terminal_fd, shell_pgid);
    
    /* Настройка обработки сигналов */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);
    
    sa.sa_handler = sigtstp_handler;
    sigaction(SIGTSTP, &sa, NULL);
    
    sa.sa_handler = sigquit_handler;
    sigaction(SIGQUIT, &sa, NULL);
    
    /* Игнорируем некоторые сигналы в shell */
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    int i = 0;
    int npipelines = 0;
    char line[1024];
    while (i < npipelines || promptline(line, sizeof(line)) > 0) {
        /* Обновляем статусы заданий */
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
