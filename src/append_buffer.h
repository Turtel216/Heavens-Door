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
