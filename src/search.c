#include "search.h"
#include "global_util.h"
#include "text_row.h"
#include "keys_and_mouse.h"
#include <stdlib.h>
#include <string.h>

static void search_callback(char *query, int key, struct Config *config)
{
	static int last_match = -1;
	static int direction = 1;

	static int saved_hl_line;
	static char *saved_hl = NULL;
	if (saved_hl) {
		memcpy(config->rows[saved_hl_line].hlight, saved_hl,
		       config->rows[saved_hl_line].render_size);

		free(saved_hl);
		saved_hl = NULL;
	}

	// Exit search on enter or escape. Update direction on arrow press
	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		return;
	} else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
		direction = 1;
	} else if (key == ARROW_LEFT || key == ARROW_UP) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1)
		direction = 1;

	int current = last_match;
	int i;

	// Search for query
	for (i = 0; i < config->num_rows; ++i) {
		current += direction;

		if (current == -1)
			current = config->num_rows - 1;
		else if (current == config->num_rows)
			current = 0;

		// Get row at index
		text_row *row = &config->rows[current];

		// Search row for query
		char *match = strstr(row->render, query);
		if (match) {
			last_match = current;

			config->cursor_y = current;
			config->cursor_x =
				render_x_to_row_x(row, match - row->render);
			config->row_offset = config->num_rows;

			saved_hl_line = current;
			saved_hl = malloc(row->render_size);
			memcpy(saved_hl, row->hlight, row->render_size);

			// Mark string to be highlighted
			memset(&row->hlight[match - row->render],
			       HL_SEARCH_MATCH, strlen(query));

			break;
		}
	}
}

void search_promt(struct Config *config)
{
	// Save initiale position, in case of search is canceled
	int saved_cx = config->cursor_x;
	int saved_cy = config->cursor_y;
	int saved_coloff = config->col_offset;
	int saved_rowoff = config->row_offset;

	// Ask user for query
	char *query = promt("Search for: %s (ESC to cancel)", search_callback);

	if (query)
		free(query);
	else { // Search canceled, reset curser
		config->cursor_x = saved_cx;
		config->cursor_y = saved_cy;
		config->col_offset = saved_coloff;
		config->row_offset = saved_rowoff;
	}
}
