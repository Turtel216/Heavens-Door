#pragma once

// Key values
enum keys {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DELETE_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

// Reads keyboard input
int read_keys(void);
// Get current position of cursor on the screen
int get_cursor_position(int *rows, int *cols);
