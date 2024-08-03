#include "heavens_door.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "key_marcos.h"

struct EditorsConfig {
	struct termios _termios;
};

struct EditorsConfig config;

static void disable_RawMode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &config._termios) == -1)
		die("tcsetattr");
}

void enable_RowMode(void)
{
	tcgetattr(STDIN_FILENO, &config._termios);
	atexit(disable_RawMode);

	struct termios raw = config._termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static char read_keys()
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	return c;
}

void process_keys()
{
	char c = read_keys();
	switch (c) {
	case CTRL_KEY('q'):
		exit(0);
		break;
	}
}

void die(const char *s)
{
	// Clear screen and position curser
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(EXIT_FAILURE);
}

void draw_rows()
{
	int y;
	for (y = 0; y < 24; y++) {
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

void refresh_screen()
{
	// Clear screen
	write(STDOUT_FILENO, "\x1b[2J", 4);
	// reposition curser
	write(STDOUT_FILENO, "\x1b[H", 3);

	draw_rows();

	write(STDOUT_FILENO, "\x1b[H", 3);
}
