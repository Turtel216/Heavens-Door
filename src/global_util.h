/*
 * Copyright (C) 2024 Dimitrios Papakonstantinou <papakonstantinou.dm@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  

 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef HEAVEN_GLOBAL_UTIL_H
#define HEAVEN_GLOBAL_UTIL_H

// Exit program with error
void die(const char *s);
// Set global status message, displayed in status bar
void set_status_message(const char *fmt, ...);
// Create promt
char *promt(char *prompt, void (*callback)(char *, int, struct Config *));

#endif // !HEAVEN_GLOBAL_UTIL_H
