#include "text_row.h"
#include "heavens_door.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

	update_syntax(row);
}

// Insert character into row
void row_insert_char(text_row *row, int at, char c)
{
	if (at < 0 || at > row->size)
		at = row->size;

	row->chars = realloc(row->chars, row->size + 2);
	if (row->chars == NULL)
		die("Error reallocating memory");

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

// Convert render_x to courser_x
int render_x_to_row_x(text_row *row, int rx)
{
	int cur_rx = 0;
	int cx;
	for (cx = 0; cx < row->size; cx++) {
		if (row->chars[cx] == '\t')
			cur_rx += (TAB_STOP - 1) - (cur_rx % TAB_STOP);

		if (++cur_rx > rx)
			return cx;
	}

	return cx;
}

// Delete character from text row
void row_delete_char(text_row *row, int at)
{
	// Check for out of bounce index
	if (at < 0 || at >= row->size)
		return;

	// Remove char from text_row
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;

	// Update row
	update_row(row);
}

// Convert highlight enum to ascii characters
int syntax_to_color(int hlight)
{
	switch (hlight) {
	case HL_NUMBER:
		return 31;
	case HL_SEARCH_MATCH:
		return 34;
	default:
		return 37;
	}
}

// Free given text_row
void free_row(text_row *row)
{
	free(row->render);
	free(row->chars);
	free(row->hlight);
}
