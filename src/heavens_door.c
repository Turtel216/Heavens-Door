#include "heavens_door.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "key_marcos.h"
#include "append_buffer.h"

struct EditorsConfig {
	int cursor_x, cursor_y;
	int screen_rows;
	int screen_cols;
	struct termios _termios;
};

enum keys { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };

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

	if (c == '\x1b') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1)
			return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1)
			return '\x1b';
		if (seq[0] == '[') {
			switch (seq[1]) {
			case 'A':
				return ARROW_UP;
			case 'B':
				return ARROW_DOWN;
			case 'C':
				return ARROW_RIGHT;
			case 'D':
				return ARROW_LEFT;
			}
		}

		return '\x1b';
	} else {
		return c;
	}
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
	config.cursor_x = 0;
	config.cursor_y = 0;

	if (get_window_size(&config.screen_rows, &config.screen_cols) == -1)
		die("get window size");
}

#define HEAVENS_DOOR_VERSION "0.1"

static void draw_rows(struct abuf *ab)
{
	int y;
	for (y = 0; y < config.screen_rows; ++y) {
		if (y == config.screen_rows / 3) {
			char welcome[80];

			int welcome_len = snprintf(
				welcome, sizeof(welcome),
				"Heaven's Door text editor -- version %s",
				HEAVENS_DOOR_VERSION);

			if (welcome_len > config.screen_cols)
				welcome_len = config.screen_cols;

			int padding = (config.screen_cols - welcome_len) / 2;
			if (padding) {
				buffer_append(ab, "~", 1);
				padding--;
			}
			while (padding--)
				buffer_append(ab, " ", 1);

			buffer_append(ab, welcome, welcome_len);
		} else {
			buffer_append(ab, "~", 1);
		}

		buffer_append(ab, "\x1b[K", 3);

		if (y < config.screen_rows - 1) {
			buffer_append(ab, "\r\n", 2);
		}
	}
}

static void move_cursor(int key)
{
	switch (key) {
	case ARROW_LEFT:
		config.cursor_x--;
		break;
	case ARROW_RIGHT:
		config.cursor_x++;
		break;
	case ARROW_UP:
		config.cursor_y--;
		break;
	case ARROW_DOWN:
		config.cursor_y++;
		break;
	}
}

void process_keys()
{
	int c = read_keys();

	switch (c) {
	case CTRL_KEY('q'):
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;
	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_LEFT:
	case ARROW_RIGHT:
		move_cursor(c);
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
	struct abuf ab = ABUF_INIT;

	buffer_append(&ab, "\x1b[?25l", 6);
	buffer_append(&ab, "\x1b[H", 3);

	draw_rows(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", config.cursor_y + 1,
		 config.cursor_x + 1);
	buffer_append(&ab, buf, strlen(buf));

	buffer_append(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);

	ab_free(&ab);
}
