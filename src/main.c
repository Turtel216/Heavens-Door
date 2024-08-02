#include <unistd.h>
#include "exit_codes.h"
#include "heavens_door.h"

int main(void)
{
	enable_RowMode();

	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
		;
	return EXIT_SUCCESS;
}
