#include <stdlib.h>
#include <unistd.h>
#include "heavens_door.h"

int main(int argc, char *argv[])
{
	enable_RowMode();
	init_editor();

	if (argc >= 2)
		open_editor(argv[1]);

	while (1) {
		refresh_screen();
		process_keys();
	}

	return EXIT_SUCCESS;
}
