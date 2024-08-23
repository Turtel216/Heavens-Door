#pragma once
#include <stddef.h>

// Tab size
#define TAB_STOP 3

// Holds meta data for each line of text
typedef struct text_row {
	size_t size; // size of each line
	char *chars; // string of characters of each line
	size_t render_size; // size of rendered line
	char *render; // actual characters drawn on screen
	unsigned char *hlight; // array containing highlighting of each line
} text_row;

// Enum containing all highlighting values
enum editor_highlight {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_SEARCH_MATCH
};

// Initialize rendered row
void update_row(text_row *row);
// Insert character into row
void row_insert_char(text_row *row, int at, char c);
// Convert courser_x to render_x
int cursor_x_to_render_x(text_row *row, int cx);
// Convert render_x to courser_x
int render_x_to_row_x(text_row *row, int rx);
// Delete character from text row
void row_delete_char(text_row *row, int at);
// Convert highlight enum to ascii characters
int syntax_to_color(int hlight);
// Free given text_row
void free_row(text_row *row);
