#include "heavens_door.h"
#include <termios.h>
#include <unistd.h>

void enable_RowMode(void)
{
	struct termios raw;

	tcgetattr(STDIN_FILENO, &raw);

	raw.c_lflag &= ~(ECHO);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
