#include "io.h"

#include <libgen.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

static char login[LOGIN_NAME_MAX];
static char hostname[MAXHOSTNAMELEN + 1];
static char prompt_char;
static char cwd[PATH_MAX];
static char *dir_name = cwd;

static int on_newline(int count, int key) {
    if (rl_end == 0 || rl_line_buffer[rl_end - 1] != '\\') {
        printf("\n");
        rl_done = 1;
        return 0;
    }
    rl_point = rl_end;
    rl_insert_text("\n");
    rl_line_buffer[rl_end - 1] = '\n';
    return 0;
}

void prompt_init() {
    prompt_char = geteuid() == 0 ? '#' : '$';
    if (getlogin_r(login, sizeof(login))) {
        perror("Could not get login");
        login[0] = 0;
    }
    if (gethostname(hostname, sizeof(hostname))) {
        hostname[0] = 0;
    }

    rl_bind_key('\r', on_newline);
    rl_bind_key('\n', on_newline);
}

static void update_cwd() {
    if (!getcwd(cwd, sizeof(cwd))) {
        cwd[0] = 0;
    }
    dir_name = basename(cwd);
}

static int make_prompt(char *buffer, int buf_len) {
    if (login[0] && hostname[0] && dir_name[0]) {
        return snprintf(buffer, buf_len, "[%s@%s %s]%c ", login, hostname,
                        dir_name, prompt_char);
    } else {
        return snprintf(buffer, buf_len, "shell%c ", prompt_char);
    }
}

static int is_blank(const char *string) {
    if (!string) {
        return 1;
    }
    for (const char *ch = string; *ch; ch++) {
        if (!isspace(*ch)) {
            return 0;
        }
    }
    return 1;
}

char *prompt_line(char *prev) {
    free(prev);

    update_cwd();
    int count = make_prompt(NULL, 0);
    char prompt[count + 1];
    make_prompt(prompt, sizeof(prompt));

    char *line = readline(prompt);
    if (!is_blank(line)) {
        add_history(line);
    }

    return line;
}
