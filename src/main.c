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

#include <stdlib.h>
#include <unistd.h>
#include "heavens_door.h"
#include "global_util.h"

int main(int argc, char *argv[])
{
	enable_RowMode();
	init_editor();

	// Check if a file path is given
	if (argc >= 2)
		open_editor(argv[1]); // open file

	set_status_message(
		"Press ctrl-q to quit | ctrl-s to save | ctrl-f to search");

	// Input loop
	while (1) {
		refresh_screen();
		process_keys();
	}

	return EXIT_SUCCESS;
}
