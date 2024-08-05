#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "heavens_door.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "append_buffer.h"

typedef struct text_row {
	size_t size;
	char *chars;
} text_row;

struct EditorsConfig {
	int cursor_x, cursor_y;
	int screen_rows;
	int screen_cols;
	size_t num_rows;
	text_row *row;
	struct termios _termios;
};

enum keys {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DELETE_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

struct EditorsConfig config;

#define CTRL_KEY(k) ((k) & 0x1f)

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

static int read_keys()
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
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
					return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
					case '1':
						return HOME_KEY;
					case '3':
						return DELETE_KEY;
					case '4':
						return END_KEY;
					case '5':
						return PAGE_UP;
					case '6':
						return PAGE_DOWN;
					case '7':
						return HOME_KEY;
					case '8':
						return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
				case 'A':
					return ARROW_UP;
				case 'B':
					return ARROW_DOWN;
				case 'C':
					return ARROW_RIGHT;
				case 'D':
					return ARROW_LEFT;
				case 'H':
					return HOME_KEY;
				case 'F':
					return END_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
			case 'H':
				return HOME_KEY;
			case 'F':
				return END_KEY;
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
	config.num_rows = 0;
	config.row = NULL;

	if (get_window_size(&config.screen_rows, &config.screen_cols) == -1)
		die("get window size");
}

static void append_row(char *s, size_t len)
{
	config.row =
		realloc(config.row, sizeof(text_row) * (config.num_rows + 1));

	int at = config.num_rows;

	config.row[at].size = len;
	config.row[at].chars = malloc(len + 1);

	memcpy(config.row[at].chars, s, len);

	config.row[at].chars[len] = '\0';
	config.num_rows++;
}

void open_editor(char *filename)
{
	FILE *fp = fopen(filename, "r");

	if (!fp)
		die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 &&
		       (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
			linelen--;
		append_row(line, linelen);
	}

	free(line);
	fclose(fp);
}

#define HEAVENS_DOOR_VERSION "0.1"

static void draw_rows(struct abuf *ab)
{
	int y;
	for (y = 0; y < config.screen_rows; ++y) {
		if (y >= config.num_rows) {
			if (config.num_rows == 0 &&
			    y == config.screen_rows / 3) {
				char welcome[80];
				int welcomelen = snprintf(
					welcome, sizeof(welcome),
					"Heaven's Door editor -- version %s",
					HEAVENS_DOOR_VERSION);

				if (welcomelen > config.screen_cols)
					welcomelen = config.screen_cols;

				int padding =
					(config.screen_cols - welcomelen) / 2;
				if (padding) {
					buffer_append(ab, "~", 1);
					padding--;
				}

				while (padding--)
					buffer_append(ab, " ", 1);

				buffer_append(ab, welcome, welcomelen);
			} else {
				buffer_append(ab, "~", 1);
			}
		} else {
			int len = config.row[y].size;
			if (len > config.screen_cols)
				len = config.screen_cols;

			buffer_append(ab, config.row[y].chars, len);
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
		if (config.cursor_x != 0)
			config.cursor_x--;
		break;
	case ARROW_RIGHT:
		if (config.cursor_x != config.screen_cols - 1)
			config.cursor_x++;
		break;
	case ARROW_UP:
		if (config.cursor_y != 0)
			config.cursor_y--;
		break;
	case ARROW_DOWN:
		if (config.cursor_y != config.screen_rows - 1)
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

	case HOME_KEY:
		config.cursor_x = 0;
		break;
	case END_KEY:
		config.cursor_x = config.screen_cols - 1;
		break;

	case PAGE_UP:
	case PAGE_DOWN: {
		int times = config.screen_rows;
		while (times--)
			move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
	} break;
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
