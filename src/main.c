#include <stdlib.h>
#include <unistd.h>
#include "heavens_door.h"

int main(int argc, char *argv[])
{
	enable_RowMode();
	init_editor();

	// Check if a file path is given
	if (argc >= 2)
		open_editor(argv[1]); // open file

	set_status_message("Press ctrl-q to quit");

	// Input loop
	while (1) {
		refresh_screen();
		process_keys();
	}

	return EXIT_SUCCESS;
}
