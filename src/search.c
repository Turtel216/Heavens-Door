#include "search.h"
#include "global_util.h"
#include "text_row.h"
#include <stdlib.h>
#include <string.h>

void search_promt(struct Config *config)
{
	// Ask user for query
	char *query = promt("Search for: %s (ESC to cancel)");
	if (query == NULL)
		return;

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
			config->num_rows = config->num_rows;

			break;
		}
	}

	free(query);
}
