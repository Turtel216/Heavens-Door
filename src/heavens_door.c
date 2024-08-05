// Necessery for file handling
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
// ###########################

// Version number displayed on home screen
#define HEAVENS_DOOR_VERSION "0.1"

#include "heavens_door.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "append_buffer.h"

// Holds meta data for each line of text
typedef struct text_row {
	size_t size; // size of each line
	char *chars; // string of characters of each line
} text_row;

// Editor internal state
struct EditorsConfig {
	int cursor_x, cursor_y; // cursor position
	int row_offset;
	int screen_rows;
	int screen_cols;
	size_t num_rows;
	text_row *rows; // Array of rows
	struct termios _termios; // structed uses for handling terminal
};

// Global editors state
struct EditorsConfig config;

// Key values
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

// Marco for checking if ctrl key is pressed
#define CTRL_KEY(k) ((k) & 0x1f)

// Disables raw mode in terminal, exits program on fail
static void disable_RawMode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &config._termios) == -1)
		die("tcsetattr");
}

// Allows editor to write in terminal
void enable_RowMode(void)
{
	tcgetattr(STDIN_FILENO, &config._termios);
	// Disable raw mode on program exit
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

// Reads keyboard input
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

// Get current position of cursor on the screen
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

// Get the users window size
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

// Initializes editor state
void init_editor()
{
	config.cursor_x = 0;
	config.cursor_y = 0;
	config.row_offset = 0;
	config.num_rows = 0;
	config.rows = NULL;

	// Set rows and collums according to screen size, exit on failure
	if (get_window_size(&config.screen_rows, &config.screen_cols) == -1)
		die("get window size");
}

// Adds a row to output string
static void append_row(char *s, size_t len)
{
	config.rows =
		realloc(config.rows, sizeof(text_row) * (config.num_rows + 1));

	int at = config.num_rows;

	config.rows[at].size = len;
	config.rows[at].chars = malloc(len + 1);

	memcpy(config.rows[at].chars, s, len);

	config.rows[at].chars[len] = '\0';
	config.num_rows++;
}

// Opens up given file
void open_editor(char *filename)
{
	FILE *fp = fopen(filename, "r");

	if (!fp)
		die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	size_t linelen;

	// Read each line and append to rows
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 &&
		       (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
			linelen--;
		append_row(line, linelen);
	}

	free(line);
	fclose(fp);
}

// Draws rows to screen
static void draw_rows(struct abuf *ab)
{
	int y;
	for (y = 0; y < config.screen_rows; ++y) {
		int file_row = y + config.row_offset;
		if (file_row >= config.num_rows) {
			if (y >= config.num_rows) {
				// At the beginning draw welcome message
				if (config.num_rows == 0 &&
				    y == config.screen_rows / 3) {
					char welcome[80];
					int welcomelen = snprintf(
						welcome, sizeof(welcome),
						"Heaven's Door editor -- version %s",
						HEAVENS_DOOR_VERSION);

					if (welcomelen > config.screen_cols)
						welcomelen = config.screen_cols;

					// Add padding to welcome message
					int padding = (config.screen_cols -
						       welcomelen) /
						      2;
					if (padding) {
						buffer_append(ab, "~", 1);
						padding--;
					}

					// Add necessery white space for padding
					while (padding--)
						buffer_append(ab, " ", 1);

					// Draw welcome message
					buffer_append(ab, welcome, welcomelen);
				} else {
					// Add '~' to empty lines
					buffer_append(ab, "~", 1);
				}
			}
		} else { // Draw text
			int len = config.rows[file_row].size;
			if (len > config.screen_cols)
				len = config.screen_cols;

			buffer_append(ab, config.rows[file_row].chars, len);
		}

		buffer_append(ab, "\x1b[K", 3);
		if (y < config.screen_rows - 1) {
			buffer_append(ab, "\r\n", 2);
		}
	}
}

// Moves cursor on screen
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
		if (config.cursor_y !=
		    0) // prevent cursor from leaving the screen
			config.cursor_y--;
		break;
	case ARROW_DOWN:
		if (config.cursor_y <
		    config.num_rows) // prevent cursor from leaving the screen
			config.cursor_y++;
		break;
	}
}

// Processes keyboard input
void process_keys()
{
	int c = read_keys();

	switch (c) {
	case CTRL_KEY('q'): // ctrl + q to quite the program
		// Clear screen and exit program
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;

	case HOME_KEY: // Jump to start of line
		config.cursor_x = 0;
		break;
	case END_KEY: // Jump to end of line
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

// Exit program with error
void die(const char *s)
{
	// Clear screen and position curser
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	// print error and exit
	perror(s);
	exit(EXIT_FAILURE);
}

// Adjust row offset in order to scroll to out of sight text
static void scroll()
{
	if (config.cursor_y < config.row_offset) {
		config.row_offset = config.cursor_y;
	}
	if (config.cursor_y >= config.row_offset + config.screen_rows) {
		config.row_offset = config.cursor_y - config.screen_rows + 1;
	}
}

// Clears screen and draws updated state
void refresh_screen()
{
	// adjust row_offset
	scroll();

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
