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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "global_util.h"
#include "keys_and_mouse.h"

// Reads keyboard input
int read_keys(void)
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}

	if (c == '\x1b') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\x1b';

		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\x1b';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
					case '1':
						return HOME_KEY;
					case '3':
						return DELETE_KEY;
					case '4':
						return END_KEY;
					case '5':
						return PAGE_UP;
					case '6':
						return PAGE_DOWN;
					case '7':
						return HOME_KEY;
					case '8':
						return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
				case 'A':
					return ARROW_UP;
				case 'B':
					return ARROW_DOWN;
				case 'C':
					return ARROW_RIGHT;
				case 'D':
					return ARROW_LEFT;
				case 'H':
					return HOME_KEY;
				case 'F':
					return END_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
			case 'H':
				return HOME_KEY;
			case 'F':
				return END_KEY;
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}

// Get current position of cursor on the screen
int get_cursor_position(int *rows, int *cols)
{
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return -1;
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
			break;
		if (buf[i] == 'R')
			break;
		i++;
	}

	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[')
		return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
		return -1;

	return 0;
}
