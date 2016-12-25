#include "types.h"
#include "user.h"
#include "x86.h"

uint
strlen(char *s)
{
	uint n;
	for (n = 0; s[n]; n++)
		;
	return n;
}

char *
strchr(const char *s, char c)
{
	for (; *s; s++)
		if (*s == c)
			return (char*)s;
	return 0;
}

void *
memset(void *dst, int c, int n)
{
	stosb(dst, c, n);
	return dst;
}

uint
puts(char *s)
{
	return write(1, s, strlen(s));
}

char *
gets(char *buf, int max)
{
	int i, cc;
	char c;

	for (i=0; i+1 < max; ) {
		cc =read(0, &c, 1);
		if (cc < 1)
			break;
		buf[i++] = c;
		if (c == '\n' || c == '\r')
			break;
	}
	buf[i] = '\0';
	return buf;
}
