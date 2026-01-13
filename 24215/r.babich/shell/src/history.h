#ifndef HISTORY_H
#define HISTORY_H

typedef struct history_entry_t {
	char line[1024];
	struct history_entry_t *next;
	struct history_entry_t *prev;
} history_entry_t;

typedef struct history_t {
	history_entry_t *start;
	history_entry_t *end;
	history_entry_t *current;
} history_t;

void add_line_to_history(const char *line, history_t *history);

char *get_prev_line(history_t *history); 
char *get_next_line(history_t *history); 

void free_history(history_t *history);

void history_init(history_t *history);

#endif // HISTORY_H

