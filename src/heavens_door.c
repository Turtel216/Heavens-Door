// Necessery for file handling
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
// ###########################

// Marcos

// Version number displayed on home screen
#define HEAVENS_DOOR_VERSION "0.5"

// Marco for checking if ctrl key is pressed
#define CTRL_KEY(k) ((k) & 0x1f)

// Marco for keeping track
// how many times quit has
// be pressed until force quite
#define QUITE_TIMES 3

// Marco for marking file as dirty aka unsaved data
#define DIRTY 1
// Marco for marking file as clean aka no unsaved data
#define CLEAN 0

//TODO bit signitures for diffrent modes
#define INSERT_MODE 00
#define NORMAL_MODE 01
#define COMMAND_MODE 11

//#######

// Standard lib
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
//#############

// Custom lib
#include "heavens_door.h"
#include "append_buffer.h"
#include "text_row.h"
#include "keys_and_mouse.h"
#include "global_util.h"
//###########

//TODO add 2 byte bit field for editor mode status
// Editor internal state
struct EditorsConfig {
	int cursor_x, cursor_y; // cursor position
	int render_x; // index of render string
	int row_offset, col_offset;
	int screen_rows, screen_cols;
	int num_rows;
	text_row *rows; // Array of rows
	unsigned int dirty : 1; // Bit flag for tracking saved/unsaved data
	char *filename; // Name of the file being displayed
	char status_msg[80]; // Global status message displayed in stutus bar
	time_t status_msg_time; // Time of the last status message
	struct termios _termios; // structed uses for handling terminal
};

// Global editors state
struct EditorsConfig config;

// Function definitions

// Adjust row offset in order to scroll to out of sight text
static void scroll(void);
//######################

// Init and Exit functions

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
void init_editor(void)
{
	config.cursor_x = 0;
	config.cursor_y = 0;
	config.render_x = 0;
	config.row_offset = 0;
	config.col_offset = 0;
	config.num_rows = 0;
	config.rows = NULL;
	config.dirty = CLEAN;
	config.filename = NULL;
	config.status_msg[0] = '\0';
	config.status_msg_time = 0;

	// Set rows and collums according to screen size, exit on failure
	if (get_window_size(&config.screen_rows, &config.screen_cols) == -1)
		die("get window size");

	// Adjest cols for status bar
	config.screen_rows -= 2;
}

//########################

// Local text row functions

// Adds a row to output string
static void append_row(int at, char *s, size_t len)
{
	if (at < 0 || at > config.num_rows)
		return;

	config.rows =
		realloc(config.rows, sizeof(text_row) * (config.num_rows + 1));
	if (config.rows == NULL)
		die("error reallocating memory");

	memmove(&config.rows[at + 1], &config.rows[at],
		sizeof(text_row) * (config.num_rows - at));

	config.rows[at].size = len;
	config.rows[at].chars = malloc(len + 1);

	if (config.rows[at].chars == NULL)
		die("error allocating memory");

	memcpy(config.rows[at].chars, s, len);

	config.rows[at].chars[len] = '\0';

	config.rows[at].render_size = 0;
	config.rows[at].render = NULL;
	update_row(&config.rows[at]);

	config.num_rows++;
	config.dirty = DIRTY;
}

// Convert a text_row into a string suitable for file input.
// Caller needs to call free on buf
static char *row_to_string(int *buf_len)
{
	int totel_len = 0;
	int j;
	for (j = 0; j < config.num_rows; j++)
		totel_len += config.rows[j].size + 1;

	*buf_len = totel_len;
	char *buf = malloc(totel_len);
	if (buf == NULL)
		die("Error allocating memory");

	char *p = buf;

	for (j = 0; j < config.num_rows; j++) {
		memcpy(p, config.rows[j].chars, config.rows[j].size);
		p += config.rows[j].size;
		*p = '\n';
		p++;
	}

	return buf;
}

// Add new string to row
void row_append_string(text_row *row, char *s, size_t len)
{
	// Allocate memory for new string
	row->chars = realloc(row->chars, row->size + len + 1);
	if (row->chars == NULL)
		die("Error reallocating memory");

	// Move string to new row
	memcpy(&row->chars[row->size], s, len);
	// Adjust row for new string
	row->size += len;
	// Append NULL terminator
	row->chars[row->size] = '\0';

	update_row(row);
	// Mark file as dirty
	config.dirty = DIRTY;
}

//#########################

// File I/O

// Opens up given file
void open_editor(char *filename)
{
	free(config.filename);
	config.filename = strdup(filename);

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
		append_row(config.num_rows, line, linelen);
	}

	free(line);
	fclose(fp);
	config.dirty = CLEAN;
}

// Save text_row contect to file
void save_to_file(void)
{
	// Check if file name has been specified
	if (config.filename == NULL)
		config.filename = promt("Save as: %s");

	// Check if promt was successful
	if (config.filename == NULL) {
		set_status_message("Save aborted");
		return;
	}

	int len;
	// Get output string
	char *buf = row_to_string(&len);

	// Open up file. Create new file if one does'nt exist
	int fd = open(config.filename, O_RDWR | O_CREAT, 0644);

	// Save to file,
	// close file and free buffer
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				set_status_message("%d bytes written to disk",
						   len);

				// Mark that there is no unsaved data
				config.dirty = CLEAN;
				return;
			}
		}
		close(fd);
	}

	// Error accured!
	free(buf);
	set_status_message("Can't save! I/O error: %s", strerror(errno));
}
//########

// Character render functions

static void insert_char(int c)
{
	if (config.cursor_y == config.num_rows) {
		append_row(config.num_rows, "", 0);
	}

	row_insert_char(&config.rows[config.cursor_y], config.cursor_x, c);
	config.dirty = DIRTY;
	config.cursor_x++;
}

static void delete_row(int at)
{
	// Check for out of bounce index
	if (at < 0 || at >= config.num_rows)
		return;

	// Delete previous row
	free_row(&config.rows[at]);
	// Move next row to current row
	memmove(&config.rows[at], &config.rows[at + 1],
		sizeof(text_row) * (config.num_rows - at - 1));

	config.num_rows--;
	config.dirty = DIRTY;
}

// Move to new line and add \n
void insert_new_line()
{
	// Check if start of line
	if (config.cursor_x == 0) {
		append_row(config.cursor_y, "", 0);
	} else {
		// Get current row
		text_row *row = &config.rows[config.cursor_y];
		// Append to next row
		append_row(config.cursor_y + 1, &row->chars[config.cursor_x],
			   row->size - config.cursor_x);

		// Update row info
		row = &config.rows[config.cursor_y];
		row->size = config.cursor_x;
		row->chars[row->size] = '\0';
		// Update rows
		update_row(row);
	}

	// Move cursor to start of next line
	config.cursor_y++;
	config.cursor_x = 0;
}

// Delete character
static void delete_char(void)
{
	// Check for out of bounce cursor
	if (config.cursor_y == config.num_rows)
		return;
	if (config.cursor_x == 0 && config.cursor_y == 0)
		return;

	// delete character from text row
	text_row *row = &config.rows[config.cursor_y];
	if (config.cursor_x > 0) { // Check for out of bounce cursor
		row_delete_char(row, config.cursor_x - 1);
		// Reposition cursor
		config.cursor_x--;
	} else {
		// Update cursor on x axis
		config.cursor_x = config.rows[config.cursor_y - 1].size;

		// Append new string to row
		row_append_string(&config.rows[config.cursor_y - 1], row->chars,
				  row->size);
		// delete old row
		delete_row(config.cursor_y);

		// Update cursor on y axis
		config.cursor_y--;
	}

	// Mark file as dirty
	config.dirty = DIRTY;
}

// Draw global status message
static void draw_message(struct abuf *ab)
{
	buffer_append(ab, "\x1b[K", 3);
	int msglen = strlen(config.status_msg);

	if (msglen > config.screen_cols)
		msglen = config.screen_cols;
	if (msglen && time(NULL) - config.status_msg_time < 5)
		buffer_append(ab, config.status_msg, msglen);
}

static void draw_status_bar(struct abuf *ab)
{
	buffer_append(ab, "\x1b[7m", 4);

	char status[80], right_status[80]; // String holding status bar info

	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
			   config.filename ? config.filename : "[No Name]",
			   config.num_rows,
			   config.dirty == DIRTY ? "(modified)" : "");

	int rlen = snprintf(right_status, sizeof(right_status), "%d/%d",
			    config.cursor_y + 1, config.num_rows);

	if (len > config.screen_cols)
		len = config.screen_cols;

	buffer_append(ab, status, len);

	while (len < config.screen_cols) {
		if (config.screen_cols - len == rlen) {
			buffer_append(ab, right_status, rlen);
			break;
		} else {
			buffer_append(ab, " ", 1);
			len++;
		}
	}

	buffer_append(ab, "\x1b[m", 3);
	buffer_append(ab, "\r\n", 2);
}

// Draws rows to screen
static void draw_rows(struct abuf *ab)
{
	for (int y = 0; y < config.screen_rows; ++y) {
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
			int len = config.rows[file_row].render_size -
				  config.col_offset;
			if (len < 0)
				len = 0;
			if (len > config.screen_cols)
				len = config.screen_cols;

			buffer_append(
				ab,
				&config.rows[file_row].render[config.col_offset],
				len);
		}

		buffer_append(ab, "\x1b[K", 3);
		buffer_append(ab, "\r\n", 2);
	}
}

// Clears screen and draws updated state
void refresh_screen(void)
{
	// adjust row_offset
	scroll();

	struct abuf ab = ABUF_INIT;

	buffer_append(&ab, "\x1b[?25l", 6);
	buffer_append(&ab, "\x1b[H", 3);

	draw_rows(&ab);
	draw_status_bar(&ab);
	draw_message(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
		 (config.cursor_y - config.row_offset) + 1,
		 (config.render_x - config.col_offset) + 1);

	buffer_append(&ab, buf, strlen(buf));

	buffer_append(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);

	ab_free(&ab);
}

//#########################

// User I/O

// Moves cursor on screen
static void move_cursor(int key)
{
	// Get current row information
	text_row *row = (config.cursor_y >= config.num_rows) ?
				NULL :
				&config.rows[config.cursor_y];

	switch (key) {
	case ARROW_LEFT:
		if (config.cursor_x != 0) {
			config.cursor_x--;
		} else if (config.cursor_y >
			   0) { // move left at the start of a line
			config.cursor_y--;
			config.cursor_x = config.rows[config.cursor_y].size;
		}
		break;
	case ARROW_RIGHT:
		if (row && config.cursor_x < row->size) {
			config.cursor_x++;
		} else if (row &&
			   config.cursor_x ==
				   row->size) { // move left at the start of a line

			config.cursor_y++;
			config.cursor_x = 0;
		}
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

	// Update row information
	row = (config.cursor_y >= config.num_rows) ?
		      NULL :
		      &config.rows[config.cursor_y];
	int row_len = row ? row->size : 0;

	// Snap cursor to end of line
	if (config.cursor_x > row_len) {
		config.cursor_x = row_len;
	}
}

// Processes keyboard input
void process_keys(void)
{
	static int quit_times = QUITE_TIMES;
	int c = read_keys();

	switch (c) {
	case '\r':
		insert_new_line();
		break;

	case CTRL_KEY('q'): // ctrl + q to quite the program
		// Check for unsaved data
		if (config.dirty == DIRTY && quit_times > 0) {
			set_status_message(
				"WARNING!!! File has unsaved changes. "
				"Press ctrl-q %d more times to quit.",
				quit_times--);
			return;
		}
		// Clear screen and exit program
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;

	case CTRL_KEY('s'):
		save_to_file();
		break;

	case HOME_KEY: // Jump to start of line
		config.cursor_x = 0;
		break;

	case END_KEY: // Jump to end of line
		if (config.cursor_y < config.num_rows)
			config.cursor_x = config.rows[config.cursor_x].size;
		break;

	case BACKSPACE:
	case CTRL_KEY('h'):
	case DELETE_KEY:
		if (c == DELETE_KEY)
			move_cursor(ARROW_RIGHT);
		delete_char();
		break;

	case PAGE_UP:
	case PAGE_DOWN: {
		if (c == PAGE_UP) {
			config.cursor_y = config.row_offset;
		} else if (c == PAGE_DOWN) {
			config.cursor_y =
				config.row_offset + config.screen_rows - 1;
			if (config.cursor_y > config.num_rows)
				config.cursor_y = config.num_rows;
		}
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

	case CTRL_KEY('l'):
	case '\x1b':
		break;

	default:
		insert_char(c);
		break;
	}

	quit_times = QUITE_TIMES;
}

// Adjust row offset in order to scroll to out of sight text
static void scroll(void)
{
	// Adjust render x
	config.render_x = 0;
	if (config.cursor_y < config.num_rows)
		config.render_x = cursor_x_to_render_x(
			&config.rows[config.cursor_y], config.cursor_x);

	if (config.cursor_y < config.row_offset) {
		config.row_offset = config.cursor_y;
	}
	if (config.cursor_y >= config.row_offset + config.screen_rows) {
		config.row_offset = config.cursor_y - config.screen_rows + 1;
	}
	if (config.render_x < config.col_offset) {
		config.col_offset = config.render_x;
	}
	if (config.render_x >= config.col_offset + config.screen_cols) {
		config.col_offset = config.render_x - config.screen_cols + 1;
	}
}
//#########

// Global util functions declarations

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

// Set global status message, displayed in status bar
void set_status_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(config.status_msg, sizeof(config.status_msg), fmt, ap);
	va_end(ap);
	config.status_msg_time = time(NULL);
}

// Create promt
char *promt(char *prompt)
{
	// Create buffer string
	size_t bufsize = 128;
	char *buf = malloc(bufsize);
	if (buf == NULL)
		die("Error allocating memory");

	size_t buflen = 0;
	buf[0] = '\0';

	//TODO add endless loop fail save counter
	while (1) {
		set_status_message(prompt, buf);
		refresh_screen();

		int c = read_keys();
		// Check if user uses deletion keys
		if (c == DELETE_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buflen != 0)
				buf[--buflen] = '\0';
		} else if (c == '\x1b') { // Check of escape key
			// Cancel promt
			set_status_message("");
			free(buf);
			return NULL;
		} else if (c == '\r') { // enter pressed, return buf
			if (buflen != 0) {
				set_status_message("");
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) { // Append to buffer
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}

			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
	}
}
//##################################
