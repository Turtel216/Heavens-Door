#include <stdlib.h>
#include <unistd.h>
#include "heavens_door.h"

int main(void)
{
	enable_RowMode();
	init_editor();

	while (1) {
		refresh_screen();
		process_keys();
	}

	return EXIT_SUCCESS;
}
