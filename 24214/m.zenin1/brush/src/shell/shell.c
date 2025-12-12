#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include "parse/parser.h"
#include "parse/user_input.h"
#include "parse/pipeline.h"
#include "parse/command.h"
#include "shell/job.h"
#include "shell/shell.h"
#include "shell/job_list.h"

#define INPUT_BUFFER_CHANK_SIZE 128

static struct termios shell_term_attrs;
static char *username = "";
static char *hostname = "";
static char prompt_char = '?';
pid_t shell_pgid;
job *current_job = NULL;
job_list active_jobs;

static void shell_wait_for_job(job *j);
static char* shell_promptline();
static void shell_reap_completed();
static void shell_report_jobs_status();
static void shell_handle_all_childs_state_change();

void shell_init(){
	if (!isatty(STDIN_FILENO)){
		fprintf(stderr, "Expected terminal as stdin\n");
		exit(-1);
	}

	shell_pgid = getpgrp();

	while (tcgetpgrp(STDIN_FILENO) != shell_pgid){
		kill(-shell_pgid, SIGTTIN);
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	shell_pgid = getpid();
	if (setpgid(shell_pgid, shell_pgid)){
		perror("Failed to create process group for shell");
		shell_exit(-1);
	}

	tcsetpgrp(STDIN_FILENO, shell_pgid);
	tcgetattr(STDIN_FILENO, &shell_term_attrs);

	username = getenv("USER");
	username = username ? username : "anon";

	hostname = malloc(sysconf(_SC_HOST_NAME_MAX));
	gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));

	prompt_char = getuid() == 0 ? '#' : '$';

	active_jobs = job_list_construct();
}

void shell_main_loop(){
	while (1){
		shell_report_jobs_status();
		shell_reap_completed();

		char *input_raw = shell_promptline();
		
		if (*input_raw == '\0'){
			printf("\n");
			free(input_raw);
			shell_exit(0);
		}
		else if (strcmp(input_raw, "\n") == 0){
			free(input_raw);
			continue;
		}

		user_input input = user_input_construct();
		parsing_return_code ret = parse_line(input_raw, &input);

		free(input_raw);

		if (ret == PARSING_ERROR){
			user_input_destruct(&input);
			continue;
		}
		else if (ret == PARSING_UNFINISHED){
			user_input_destruct(&input);
			fprintf(stderr, "Unexpectedly not whole input is parsed\n");
			continue;
		}

#ifdef DEBUG
		fprintf(stderr, "pipelines read: %zu\n", input.pipelines.size);
#endif
		job *new_jobs_start = NULL;

		for (size_t i = 0; i < input.pipelines.size; i++){

#ifdef DEBUG
			fprintf(stderr, "\ncommands in %zu-th pipeline: %zu\n", i, ((pipeline*) input.pipelines.arr[i])->commands.size);
			fprintf(stderr, "original line: \"%s\"\n", ((pipeline*) input.pipelines.arr[i])->original_line);

			pipeline *pipl = (pipeline*) input.pipelines.arr[i];
			if (!pipl->foreground){
				fprintf(stderr, "Pipeline is background job\n");
			}
			if (pipl->infile){
				fprintf(stderr, "Input redir: \"%s\"\n", pipl->infile);
			}
			if (pipl->outfile){
				fprintf(stderr, "Output redir: \"%s\"\n", pipl->outfile);
			}
			if (pipl->appfile){
				fprintf(stderr, "Append redir: \"%s\"\n", pipl->appfile);
			}
			fprintf(stderr, "\n");

			for (size_t j = 0; j < pipl->commands.size; j++){
				fprintf(stderr, "-------------------------------------------\n");
				command *cmd = (command*) pipl->commands.arr[j];

				for (size_t k = 0; k < cmd->args.size; k++){
					char *arg = (char*) cmd->args.arr[k];
					fprintf(stderr, "\t\"%s\"\n", arg);
				}

			}
#endif

			job *joba = job_list_add(&active_jobs,
						 job_get_from_pipeline((pipeline*) input.pipelines.arr[i]));

			new_jobs_start = new_jobs_start ? new_jobs_start : joba;

		}

		user_input_destruct(&input);

		while (new_jobs_start){
			job_launch(new_jobs_start);

			new_jobs_start = new_jobs_start->next;
		}

	}
}

void shell_exit(int code){
	job_list_destruct(&active_jobs);
	free(hostname);
	exit(code);
}

void shell_print_job_info(job *j){
	fprintf(stderr, "[%zu]%c %s\t%s\n", j->index,
					    j == current_job ? '+' : ' ',
		      			    job_get_status_string(j),
					    j->original_line ? j->original_line : "");
}

void shell_print_all_jobs(){
	job *cursor = active_jobs.start->next;

	while (cursor != NULL){
		if (cursor->status != NOT_LAUNCHED){
			shell_print_job_info(cursor);
		}
		cursor = cursor->next;
	}
}

void shell_put_job_on_background(job *j){

	shell_handle_all_childs_state_change();
	if (kill(-j->pgid, SIGCONT)){
		perror("Failed to continue background job");
	}
}

void shell_put_job_on_foreground(job *j){
	current_job = j;
	j->foreground = true;

	tcsetpgrp(STDIN_FILENO, j->pgid);
	tcsetattr(STDIN_FILENO, TCSADRAIN, &j->term_attrs);

	shell_handle_all_childs_state_change();
	if (kill(-j->pgid, SIGCONT)){
		perror("Failed to continue foreground job");
	}

	shell_wait_for_job(j);

	tcsetpgrp(STDIN_FILENO, shell_pgid);

	tcgetattr(STDIN_FILENO, &j->term_attrs);
	if (j->status == STOPPED){
		tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_term_attrs);
		j->foreground = false;
	}
	else if (job_is_terminated_by_sig(j)){
		tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_term_attrs);
	}
	else{
		shell_term_attrs = j->term_attrs;
		tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_term_attrs);
	}
}

static void shell_wait_for_job(job *j){
	int status;
	pid_t child_pid;
	do{
		child_pid = waitpid(-j->pgid, &status, WUNTRACED);

		if (child_pid == -1){
			break;
		}
		
		job_list_set_process_status(&active_jobs, child_pid, status);
	} while (j->status != STOPPED && j->status != COMPLETED);
}

static char* shell_promptline(){
	fprintf(stdout, "%s@%s%c ", username, hostname, prompt_char);

	char *buffer = malloc(INPUT_BUFFER_CHANK_SIZE * sizeof(char));
	*buffer = '\0';

	size_t chars_readen = 0;
	size_t buffer_size = INPUT_BUFFER_CHANK_SIZE;

	bool escaping_by_backslash = false, escaping_by_quotes = false;

	while (1){
		if (chars_readen + 1 == buffer_size){
			char *tmp = realloc(buffer, buffer_size + INPUT_BUFFER_CHANK_SIZE);
			if (tmp == NULL){
				free(buffer);
				perror("Failed to enlarge input buffer size");
				exit(-1);
			}

			buffer = tmp;
			buffer_size += INPUT_BUFFER_CHANK_SIZE;
		}

		errno = 0;
		if (fgets(buffer + chars_readen, buffer_size - chars_readen, stdin) == NULL){
			if (errno){
				perror("Failed to read input");
				shell_exit(-1);
			}
			else{
				break;
			}
		}

		size_t new_chars = strlen(buffer + chars_readen);

		// If end isn't LF -> whole line wasn't readen into buffer
		if (buffer[chars_readen + new_chars - 1] != '\n'){
			chars_readen += new_chars;
			continue;
		}

		// Check if LF at the end escaped
		for (size_t i = chars_readen; i < chars_readen + new_chars; i++){
			if (buffer[i] == '\\' && !escaping_by_backslash){
				escaping_by_backslash = true;
				continue;
			}
			else if (buffer[i] == '"' && !escaping_by_backslash){
				escaping_by_quotes = !escaping_by_quotes;
			}

			if (escaping_by_backslash){
				escaping_by_backslash = false;
			}
		}

		// If LF isn't escaped
		if (!escaping_by_backslash && !escaping_by_quotes){
			break;
		}

		chars_readen += new_chars;

		fprintf(stdout, "> ");
	}

	return buffer;
}

static void shell_reap_completed(){
	job *cursor = active_jobs.start;

	while (cursor->next){
		if (cursor->next->status == COMPLETED){
			if (cursor->next == current_job){
				current_job = NULL;
			}
			job_list_remove_next(&active_jobs, cursor);
		}
		else{
			cursor = cursor->next;
		}
	}
}

static void shell_report_jobs_status(){
	shell_handle_all_childs_state_change();

	job *cursor = active_jobs.start->next;

	while (cursor){
		if (!cursor->notified){
			cursor->notified = true;
			if (!cursor->foreground){
				shell_print_job_info(cursor);
			}
		}
		cursor = cursor->next;
	}
}

static void shell_handle_all_childs_state_change(){
	pid_t child_pid;
	int status;

	while (1){
		child_pid = waitpid(-1, &status, WUNTRACED | WNOHANG | WCONTINUED);

		if (child_pid <= 0){
			break;
		}

		job_list_set_process_status(&active_jobs, child_pid, status);
	}
}

