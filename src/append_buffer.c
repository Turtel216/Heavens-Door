#include <stdlib.h>
#include <string.h>
#include "append_buffer.h"

#define ABUF_INIT { NULL, 0 }

void buffer_append(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL)
		return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void ab_free(struct abuf *ab)
{
	free(ab->b);
}
