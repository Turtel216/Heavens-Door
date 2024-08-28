/*
 * Copyright (C) 2024 Dimitrios Papakonstantinou <papakonstantinou.dm@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  

 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef HEAVEN_HEAVENS_DOOR_H
#define HEAVEN_HEAVENS_DOOR_H

#include <termios.h>
#include <time.h>
#include "text_row.h"

//TODO add 2 byte bit field for editor mode status
// Editor internal state
struct Config {
	int cursor_x, cursor_y; // cursor position
	int render_x; // index of render string
	int row_offset, col_offset;
	int screen_rows, screen_cols;
	int num_rows;
	text_row *rows; // Array of rows
	unsigned int dirty : 1; // Bit flag for tracking saved/unsaved data
	char *filename; // Name of the file being displayed
	char status_msg[80]; // Global status message displayed in stutus bar
	time_t status_msg_time; // Time of the last status message
	struct syntax *syntax;
	struct termios _termios; // structed uses for handling terminal
};

struct syntax {
	char *filetype;
	char **filematch;
	char *singleline_comment_start;
	char **keywords;
	int flags;
};

// Allows editor to write in terminal
void enable_RowMode(void);
// Exit program with error
void die(const char *s);
// Reads processes keyboard input
void process_keys(void);
// Clears screen and draws updated state
void refresh_screen(void);
// Initializes editor state
void init_editor(void);
// Opens up given file
void open_editor(char *filename);
void update_syntax(text_row *row);

#endif // !HEAVEN_HEAVENS_DOOR_H
