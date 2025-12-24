#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void history_init(history_t *history) {
	*history = (history_t) {
		.start = NULL,
		.end = NULL,
		.current = NULL
	};
}

void add_line_to_history(const char *line, history_t *history) {
	history_entry_t *new_entry = malloc(sizeof(history_entry_t));
	if (!new_entry) {
		perror("Failed to allocate memory for history");
    return;
	}
	strncpy(new_entry->line, line, strlen(line));
	new_entry->prev = history->end;
	new_entry->next = NULL;

	if (history->end == NULL) {
		history->start = new_entry;
		history->end = new_entry;
	} else {
		history->end->next = new_entry;
		history->end = new_entry;

	}
	history->current = NULL;
}

char *get_prev_line(history_t *history) {
	if (history->end == NULL) {
		return NULL;
	}

	if (history->current == NULL) {
		history->current = history->end;
	} else if (history->current->prev != NULL) {
		history->current = history->current->prev;
	} else {
		return history->current->line;
	}
	return history->current->line;
}

char *get_next_line(history_t *history) {
	if (history->current == NULL) {
		return NULL;
	}

	if (history->current->next != NULL) {
		history->current = history->current->next;
		return history->current->line;
	} 		
	history->current = NULL;
	return NULL;
}

void free_history(history_t *history) {
	 history_entry_t* current = history->start;
    
    while (current != NULL) {
        history_entry_t* next = current->next;
        free(current);
        current = next;
    }
    
    history->start = NULL;
    history->end = NULL;
    history->current = NULL;
		if (!history) {
			return;
		}
}
