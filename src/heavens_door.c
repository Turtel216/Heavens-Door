#include "heavens_door.h"
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios _termios;

static void disable_RawMode(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &_termios);
}

void enable_RowMode(void)
{
	tcgetattr(STDIN_FILENO, &_termios);
	atexit(disable_RawMode);

	struct termios raw = _termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
