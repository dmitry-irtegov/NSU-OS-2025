#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include "builtins.h"
#include "job.h"
#include "job_list.h"
#include "pipeline.h"


int main(int argc, char *argv[]) {
	char line[1024];	
	char prompt[50];
	job_list_t list;
	job_list_init(&list);

	signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
	
	sprintf(prompt, "[%s] ", argv[0]);

	while (promptline(prompt, line, sizeof(line)) > 0) {
		static pipeline_t pipeline;
    check_background_jobs(&list);

		if (parse_pipeline(line, &pipeline) <= 0) {
    	continue;
    }
		if (try_execute_builtin(&pipeline, &list)) {
			continue;
		}

    job_t *job = job_init();
    if (!job) {
			continue;
		}
		job_list_add(&list, job);
		launch_job(&list, job, &pipeline);
	}
	return 0;
}

