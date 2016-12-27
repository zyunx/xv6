#include <defs.h>
#include <types.h>
#include <x86.h>

void *
memset(void *dst, int c, uint n)
{
	char *d;
/*
	if (((uint) dst) % 4 == 0 && n % 4 == 0) {
		c &= 0xff;
		stosl(dst, c<<24 | c<<16 | c<<8 | c, n/4);
	} else
		stosb(dst, c, n);
		*/
	d = dst;
	while (n-- > 0)
		*d++ = c;
	return dst;
}

void *
memmove(void *dst, const void *src, uint n)
{
	const char *s;
	char *d;

	s = src;
	d = dst;
	if (s < d && s + n > d) {
		s += n;
		d += n;
		while (n-- > 0)
			*--d = *--s;
	} else {
		while (n-- > 0)
			*d++ = *s++;
	}

	return dst;
}

int strncmp(const char *p, const char *q, uint n)
{
	while (n > 0 && *p && *p == *q)
		n--, p++, q++;
	if (n == 0)
		return 0;
	return (uchar)*p - (uchar)*q;
}

char *
strncpy(char *s, const char *t, int n)
{
	char *os;

	os = s;
	while (n-- > 0 && (*s++ = *t++) != 0)
		;
	while (n-- > 0)
		*s++ = 0;
	return os;
}

// Lick strncpy bu guaranteed to NUL-terminate.
char *
safestrcpy(char *s, const char *t, int n)
{
	char *os;

	os = s;
	if (n <= 0)
		return os;
	while (--n > 0 && (*s++ = *t++) != 0)
		;
	*s = 0;
	return os;
}

int strlen(const char *s)
{
	int n;

	for (n = 0; s[n]; n++)
		;
	return n;
}