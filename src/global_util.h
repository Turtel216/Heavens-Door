#pragma once

// Exit program with error
void die(const char *s);
// Set global status message, displayed in status bar
void set_status_message(const char *fmt, ...);
// Create promt
char *promt(char *prompt, void (*callback)(char *, int, struct Config *));
