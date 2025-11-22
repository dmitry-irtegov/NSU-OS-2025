#pragma once

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include "shell/job.h"
#include "shell/job_list.h"

extern job *current_job;
extern job_list active_jobs;
extern pid_t shell_pgid;

void shell_init();
void shell_main_loop();
void shell_exit(int code);

void shell_print_job_info(job *j);
void shell_print_all_jobs();
void shell_put_job_on_background(job *j);
void shell_put_job_on_foreground(job *j);

