#include "core/shell.h"

static pid_t shell_pgid;
static struct termios shell_tmodes;
static task_list_t task_list;
static int shell_terminal;
static int shell_is_interactive;

void shell_init(void) {
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);
    
    if (shell_is_interactive) {

        shell_pgid = getpgrp();

        while (tcgetpgrp(shell_terminal) != shell_pgid){
		    kill(-shell_pgid, SIGTTIN);
	    }

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        shell_pgid = getpid();

        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put shell in its own process group");
            exit(EXIT_FAILURE);
        }

        if (tcsetpgrp(shell_terminal, shell_pgid) < 0) {
            perror("tcsetpgrp (shell)");
            exit(EXIT_FAILURE);
        }


        if (tcgetattr(shell_terminal, &shell_tmodes) < 0) {
            perror("tcgetattr (shell)");
            exit(EXIT_FAILURE);
        }
    }

    init_task_list(&task_list);
}

pid_t shell_get_pgid(void) {
    return shell_pgid;
}

struct termios* shell_get_tmodes(void) {
    return &shell_tmodes;
}

task_list_t* shell_get_tasks(void) {
    return &task_list;
}

void shell_run(void) {
    shell_init();
    
    while (1) {
        shell_check_background_tasks();
        
        char* input = read_line();
        if (!input) {
            break;
        }
        
        task_manager_t* manager = parse_line(input);
        free(input);
        
        if (!manager) {
            continue;
        }

        for (size_t i = 0; i < manager->count; i++) {
            pipeline_t* pipeline = manager->tasks[i];
            
            if (pipeline->count == 1 && !pipeline->input_file && 
                !pipeline->output_file && !pipeline->append_file) {
                command_t* cmd = pipeline->cmds[0];
                if (shell_execute_builtin(cmd->argv[0], cmd->argv, cmd->argc)) {
                    continue;
                }
            }

            task_t* task = create_task(*pipeline);
            if (task) {
                add_task(&task_list, task);
                launch_task(task);
            }
        }
        
        destroy_task_manager(manager);
    }
    
    destroy_task_list(&task_list);
}

void shell_check_background_tasks(void) {
    task_t* task = task_list.first;
    while (task) {
        task_t* next = task->next;
        
        if (task->status == TASK_RUNNING || task->status == TASK_STOPPED) {
            int status;
            pid_t pid = waitpid(-task->pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            
            if (pid > 0) {
                for (size_t i = 0; i < task->process_count; i++) {
                    if (task->processes[i]->pid == pid) {
                        if (WIFEXITED(status)) {
                            task->processes[i]->status = PROCESS_COMPLETED;
                        } else if (WIFSIGNALED(status)) {
                            task->processes[i]->status = PROCESS_COMPLETED;
                        } else if (WIFSTOPPED(status)) {
                            task->processes[i]->status = PROCESS_STOPPED;
                        } else if (WIFCONTINUED(status)) {
                            task->processes[i]->status = PROCESS_RUNNING;
                        }
                        break;
                    }
                }

                int all_completed = 1;
                int any_stopped = 0;
                for (size_t i = 0; i < task->process_count; i++) {
                    if (task->processes[i]->status != PROCESS_COMPLETED) {
                        all_completed = 0;
                    }
                    if (task->processes[i]->status == PROCESS_STOPPED) {
                        any_stopped = 1;
                    }
                }
                
                if (all_completed) {
                    task->status = TASK_COMPLETED;
                    if (task->is_background) {
                        printf("\n[%zu]+  Done                    ", task->task_id);
                        for (size_t i = 0; i < task->process_count; i++) {
                            if (i > 0) printf(" | ");
                            printf("%s", task->processes[i]->argv[0]);
                        }
                        printf("\n");
                        task->notify = 1;
                    }
                } else if (any_stopped) {
                    task->status = TASK_STOPPED;
                }
            }
        }
        
        task = next;
    }
    
    cleanup_completed_tasks(&task_list);
}

int shell_execute_builtin(char* cmd, char** argv, int argc) {
    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    } else if (strcmp(cmd, "cd") == 0) {
        if (argc < 2) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } else if (strcmp(cmd, "jobs") == 0) {
        print_all_tasks(&task_list);
        cleanup_completed_tasks(&task_list);
        return 1;
    } else if (strcmp(cmd, "fg") == 0) {
        size_t job_id = 1;
        if (argc > 1) {
            job_id = atoi(argv[1]);
        }
        task_t* task = find_task_by_id(&task_list, job_id);
        if (task) {
            for (size_t i = 0; i < task->process_count; i++) {
                if (i > 0) printf(" | ");
                printf("%s", task->processes[i]->argv[0]);
            }
            printf("\n");
            put_task_in_fg(task, task->status == TASK_STOPPED);
        } else {
            fprintf(stderr, "fg: job %zu not found\n", job_id);
        }
        return 1;
    } else if (strcmp(cmd, "bg") == 0) {
        size_t job_id = 1;
        if (argc > 1) {
            job_id = atoi(argv[1]);
        }
        task_t* task = find_task_by_id(&task_list, job_id);
        if (task) {
            put_task_in_bg(task, task->status == TASK_STOPPED);
        } else {
            fprintf(stderr, "bg: job %zu not found\n", job_id);
        }
        return 1;
    }
    return 0;
}
