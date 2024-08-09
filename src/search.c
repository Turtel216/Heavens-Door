#include "search.h"
#include "global_util.h"
#include "text_row.h"
#include <stdlib.h>
#include <string.h>

static void search_callback(char *query, int key, struct Config *config)
{
	// Exit search on enter or escape
	if (key == '\r' || key == '\x1b') {
		return;
	}

	// Search for query
	int i;
	for (i = 0; i < config->num_rows; i++) {
		// Get row at index
		text_row *row = &config->rows[i];

		// Search row for query
		char *match = strstr(row->render, query);

		// On match found move curser to position of match
		if (match) {
			config->cursor_y = i;
			config->cursor_x =
				render_x_to_row_x(row, match - row->render);
			config->row_offset = config->num_rows;

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
