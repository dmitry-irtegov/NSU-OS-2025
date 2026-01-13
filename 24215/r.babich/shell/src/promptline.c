#include "history.h"
#include "pipeline.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>

struct termios original;

int is_blank(const char *str) {
    if (!str) return 1;
    
    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}


void enable_raw_mode() {
	struct termios raw;
  tcgetattr(STDIN_FILENO, &original);
  raw = original;

  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_cc[VMIN] = 1;  
	raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

void handle_arrow_key(char *prompt, char *line, int *pos, int *len, history_t *history) {
  char seq[2];
	read(0, &seq, 2);

	char *history_line = NULL;
		if (seq[0] == '[') {
			switch (seq[1]) {
				case 'A':
					history_line = get_prev_line(history);
					break;
				case 'B':
					history_line = get_next_line(history);
					break;
			}
		}
	if (history_line != NULL) {
    strcpy(line, history_line);
		*len = strlen(line);
		*pos = *len;
		line[*len] = '\0';

		write(STDOUT_FILENO, "\r\033[K", 5);
		write(1, prompt, strlen(prompt));
		write(1, line, strlen(line));
	}
}


int promptline(char *prompt, char *line, int sizline, history_t *history) {
	enable_raw_mode();
	int n = 0;
	char c;
	int pos = 0;

	write(1, prompt, strlen(prompt));

	while(1) {
		read(STDIN_FILENO, &c, 1);

		if (c == 4) {
			disable_raw_mode();
			return 0;
		}

		if (c == 127 || c == '\b') {
      if (pos > 0 && n > 0) {
        memmove(&line[pos-1], &line[pos], n - pos + 1);
        n--;
        pos--;
      	write(STDOUT_FILENO, "\b \b", 3);
      }
      continue;
    }

		if (c == '\x1b') {
			handle_arrow_key(prompt, line, &pos, &n,  history);
			continue;
		}

		if (c == '\n' || c == '\r') {
			if (!is_blank(line)) {
       	add_line_to_history(line, history);
			}

			line[n] = '\n';
			n++;
			line[n] = '\0';
			write(STDOUT_FILENO, "\n", 1);
			if (*(line + n - 2) == '\\' && *(line + n - 1) == '\n') {
				*(line + n - 1) = '\0';
				*(line + n - 2) = ' ';
      	n--;
      	write(STDOUT_FILENO, "> ", 2);
				pos = n;
				continue;	
			}
			break;
		}

		if (isprint(c)) {
			line[pos] = c;
      n++;
      pos++;
			*(line + n) = '\0';
    	write(STDOUT_FILENO, &c, 1);
		}
	}
	disable_raw_mode();
	return(n);	
}

