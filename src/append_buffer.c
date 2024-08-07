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
