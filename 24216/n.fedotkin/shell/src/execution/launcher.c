#include "execution/launcher.h"
#include "scheduler/task_list.h"

int launch_task(task_t* task) {
    if (!task || task->process_count == 0) {
        set_error(ERROR_SYNTAX, "launch_task: Invalid task");
        return -1;
    }

    int* pipes = malloc(2 * (task->process_count - 1) * sizeof(int));
    if (!pipes) {
        set_error(ERROR_MEMORY_ALLOCATION, "launch_task: Memory allocation failed for pipes");
        return -1;
    }
    int pipe_count = 0;

    for (size_t i = 0; i < task->process_count - 1; i++) {
        if (pipe(pipes + i * 2) < 0) {
            perror("pipe");
            close_pipes(pipes, pipe_count);
            free(pipes);
            return -1;
        }
        pipe_count += 2;
    }

    for (size_t i = 0; i < task->process_count; i++) {
        process_t* process = task->processes[i];
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close_pipes(pipes, pipe_count);
            if (task->pgid > 0) {
                kill(-task->pgid, SIGKILL);
            }
            free(pipes);
            return -1;
        } else if (pid == 0) {
            // Child process
            
            if (task->pgid == 0) {
                task->pgid = getpid();
            }
            if (setpgid(0, task->pgid) < 0) {
                perror("setpgid");
                exit(EXIT_FAILURE);
            }

            if (task->is_background) {
                signal(SIGINT, SIG_IGN);
                signal(SIGQUIT, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
            } else {
                signal(SIGTSTP, SIG_DFL);
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
            }

            int input_fd = STDIN_FILENO;
            int output_fd = STDOUT_FILENO;

            if (i > 0) {
                input_fd = pipes[(i - 1) * 2];
            } else if (task->input_file) {
                input_fd = open(task->input_file, O_RDONLY);
                if (input_fd < 0) {
                    perror("open input_file");
                    exit(EXIT_FAILURE);
                }
            }

            if (i < task->process_count - 1) {
                output_fd = pipes[i * 2 + 1];
            } else if (task->output_file) {
                output_fd = open(task->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd < 0) {
                    perror("open output_file");
                    exit(EXIT_FAILURE);
                }
            } else if (task->append_file) {
                output_fd = open(task->append_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (output_fd < 0) {
                    perror("open append_file");
                    exit(EXIT_FAILURE);
                }
            }

            if (input_fd != STDIN_FILENO) {
                if (dup2(input_fd, STDIN_FILENO) < 0) {
                    perror("dup2 (stdin)");
                    exit(EXIT_FAILURE);
                }
                close(input_fd);
            }
            if (output_fd != STDOUT_FILENO) {
                if (dup2(output_fd, STDOUT_FILENO) < 0) {
                    perror("dup2 (stdout)");
                    exit(EXIT_FAILURE);
                }
                close(output_fd);
            }

            close_pipes(pipes, pipe_count);
            free(pipes);

            execvp(process->argv[0], process->argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        // Parent process

        process->pid = pid;
        process->status = PROCESS_RUNNING;

        if (task->pgid == 0) {
            task->pgid = pid;
        }
        if (setpgid(pid, task->pgid) < 0) {
            perror("setpgid");
            close_pipes(pipes, pipe_count);
            free(pipes);
            return -1;
        }
    }
    close_pipes(pipes, pipe_count);
    free(pipes);
    task->status = TASK_RUNNING;

    if (task->is_background) {
        printf("[%zu] %d\n", task->task_id, (int)task->pgid);
        put_task_in_bg(task, 0);
    } else {
        put_task_in_fg(task, 0);
    }

    return 0;
}

void close_pipes(int* pipes, int count) {
    for (int i = 0; i < count; i++) {
        if (pipes[i] >= 0) {
            close(pipes[i]);
        }
    }
}

void put_task_in_fg(task_t* task, int cont) {
    if (!task) return;

    if (tcsetpgrp(STDIN_FILENO, task->pgid) < 0) {
        perror("tcsetpgrp (task)");
        return;
    }

    if (cont) {
        tcsetattr(STDIN_FILENO, TCSADRAIN, &task->tmodes);
        kill(-task->pgid, SIGCONT);
        task->status = TASK_RUNNING;
    }

    wait_for_task(task);

    if (tcsetpgrp(STDIN_FILENO, shell_get_pgid()) < 0) {
        perror("tcsetpgrp (shell)");
    }

    if (tcgetattr(STDIN_FILENO, &task->tmodes) < 0) {
        perror("tcgetattr (task)");
    }

    if (tcsetattr(STDIN_FILENO, TCSADRAIN, shell_get_tmodes()) < 0) {
        perror("tcsetattr (shell)");
    }

    if (task->status == TASK_COMPLETED) {
        task_list_t* tasks = shell_get_tasks();
        remove_task(tasks, task);
        destroy_task(task);
        if (tasks->count == 0) {
            tasks->next_task_id = 1;
        }
    }
}

void put_task_in_bg(task_t* task, int cont) {
    if (!task) return;

    if (cont) {
        kill(-task->pgid, SIGCONT);
        task->status = TASK_RUNNING;
        printf("[%zu]   ", task->task_id);
        for (size_t i = 0; i < task->process_count; i++) {
            if (i > 0) printf(" | ");
            printf("%s", task->processes[i]->argv[0]);
        }
        printf(" &\n");
    }
}

void wait_for_task(task_t* task) {
    if (!task) return;

    int status;
    pid_t pid;

    while (1) {
        pid = waitpid(-task->pgid, &status, WUNTRACED);

        if (pid <= 0) {
            break;
        }

        for (size_t i = 0; i < task->process_count; i++) {
            if (task->processes[i]->pid == pid) {
                
                if (WIFEXITED(status)) {
                    task->processes[i]->status = PROCESS_COMPLETED;
                } 
                else if (WIFSIGNALED(status)) {
                    task->processes[i]->status = PROCESS_COMPLETED;
                } 
                else if (WIFSTOPPED(status)) {
                    task->processes[i]->status = PROCESS_STOPPED;
                    
                    fprintf(stderr, "\n[%zu]  Stopped                 ", task->task_id);
                    for (size_t j = 0; j < task->process_count; j++) {
                        if (j > 0) fprintf(stderr, " | ");
                        fprintf(stderr, "%s", task->processes[j]->argv[0]);
                    }
                    fprintf(stderr, "\n");
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
            break;
        } 
        else if (any_stopped) {
            task->status = TASK_STOPPED;
            break;
        }
    }
}