#include "heavens_door.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "key_marcos.h"

struct EditorsConfig {
	int screen_rows;
	int screen_cols;
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

static int get_cursor_position(int *rows, int *cols)
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

static int get_window_size(int *rows, int *cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return -1;
		return get_cursor_position(rows, cols);

	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

void init_editor()
{
	if (get_window_size(&config.screen_rows, &config.screen_cols) == -1)
		die("getWindowSize");
}

static void draw_rows()
{
	int y;
	for (y = 0; y < config.screen_rows; y++) {
		write(STDOUT_FILENO, "~\r\n", 3);
	}
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

void refresh_screen()
{
	// Clear screen
	write(STDOUT_FILENO, "\x1b[2J", 4);
	// reposition curser
	write(STDOUT_FILENO, "\x1b[H", 3);

	draw_rows();

	write(STDOUT_FILENO, "\x1b[H", 3);
}
