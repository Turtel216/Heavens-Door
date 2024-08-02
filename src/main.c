#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include "heavens_door.h"

int main(void)
{
	enable_RowMode();

	while (1) {
		char c = '\0';

		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
			die("read");

		if (iscntrl(c)) {
			printf("%d\r\n", c);
		} else {
			printf("%d ('%c')\r\n", c, c);
		}

		if (c == 'q')
			break;
	}

	return EXIT_SUCCESS;
}
