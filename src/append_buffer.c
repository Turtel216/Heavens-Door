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

#include <stdlib.h>
#include <string.h>
#include "append_buffer.h"

// append to output buffer
void buffer_append(struct abuf *ab, const char *s, int len)
{
	// allocate new chunk of memory
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL)
		return;

	// Copy old state into new state and update buffer
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

// Free output buffer
inline void ab_free(struct abuf *ab)
{
	free(ab->b);
}
