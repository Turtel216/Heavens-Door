#include "text_row.h"
#include "heavens_door.h"
#include <stdlib.h>
#include <string.h>

// Initialize rendered row
void update_row(text_row *row)
{
	// count number of tabs
	int tabs = 0;
	for (int j = 0; j < row->size; ++j)
		if (row->chars[j] == '\t')
			tabs++;

	// Allocate memory for rendered string
	free(row->render);
	row->render = malloc(row->size + tabs * (TAB_STOP - 1) + 1);

	if (row->render == NULL)
		die("Error allocating memory");

	// Initialize rendered string
	int idx = 0; // tracks number of characters
	for (int j = 0; j < row->size; ++j) {
		if (row->chars[j] == '\t') { // render tabs to string
			row->render[idx++] = ' ';

			while (idx % TAB_STOP != 0)
				row->render[idx++] = ' ';
		} else {
			row->render[idx++] = row->chars[j];
		}
	}

	// At NULL terminator at the end
	row->render[idx] = '\0';
	row->render_size = idx;
}

// Insert character into row
void row_insert_char(text_row *row, int at, char c)
{
	if (at < 0 || at > row->size)
		at = row->size;

	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);

	row->size++;
	row->chars[at] = c;

	update_row(row);
}

// Convert courser_x to render_x
int cursor_x_to_render_x(text_row *row, int cx)
{
	int rx = 0;
	for (int j = 0; j < cx; ++j) {
		if (row->chars[j] == '\t')
			rx += (TAB_STOP - 1) - (rx % TAB_STOP);
		rx++;
	}

	return rx;
}
