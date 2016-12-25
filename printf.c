#include "types.h"
#include "stat.h"
#include "user.h"

static void
putc(int fd, char c)
{
	write(fd, &c, 1);
}
/*
static int
puts(int fd, char *s)
{
	return write(fd, s, strlen(s));
}
*/
static void
printint(int fd, int xx, int base, int sign)
{
	static char digits[] = "0123456789abcdef";
	char buf[33];
	int i;
	uint x;

	if (sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;
	
	i = 0;

	do {
		buf[i++] = digits[x % base];
	} while ((x /= base) != 0);

	if (sign)
		buf[i++] = '-';
	
	while (--i >= 0)
		putc(fd, buf[i]);

}

void printf(int fd, char *fmt, ...)
{
	char c, *s;
	uint *argp;
	int i;

	argp = (uint *)(void *)(&fmt+1);
	for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
		if (c != '%') {
			putc(fd, c);
			continue;
		}

		c = fmt[++i] & 0xff;
		if (c == 0)
			break;
		switch (c) {
		case 'c':
			putc(fd, *argp++);
			break;
		case 'd':
			printint(fd, *argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(fd, *argp++, 16, 0);
			break;
		case 's':
			if ((s = (char*)*argp++) == 0)
				s = "(null)";
			for (; *s; s++)
				putc(fd, *s);
			break;
		case '%':
			putc(fd, '%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			putc(fd, '%');
			putc(fd, c);
			break;
		}
	}
}


