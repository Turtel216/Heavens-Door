// Allows editor to write in terminal
void enable_RowMode(void);
// Exit program with error
void die(const char *s);
// Reads processes keyboard input
void process_keys();
// Clears screen and draws updated state
void refresh_screen();
// Initializes editor state
void init_editor();
// Opens up given file
void open_editor(char *filename);
