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

#pragma once

// Struct holding output buffer
struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT { NULL, 0 }

// append to output buffer
void buffer_append(struct abuf *ab, const char *s, int len);
// Free output buffer
void ab_free(struct abuf *ab);
