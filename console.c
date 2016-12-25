#include "defs.h"
#include "types.h"
#include "x86.h"
#include "memlayout.h"
#include "spinlock.h"
#include "kbd.h"
#include "proc.h"
#include "traps.h"

#define	BACKSPACE	0x100
#define	CRTPORT		0x3d4
#define CRTATTR		0x0700
static ushort *crt = (ushort*) P2V(0xb8000);	// CGA memory

struct console {
	struct spinlock lock;
	int locking;
} cons;

static void consputc(int c);

static void printint(int xx, int base, int sign)
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
		consputc(buf[i]);

}

void cprintf(char *fmt, ...)
{
	char c, *s, locking;
	uint *argp;
	int i;

	locking = cons.locking;
	if (locking)
		acquire(&cons.lock);

	argp = (uint *)(void *)(&fmt+1);
	for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
		if (c != '%') {
			consputc(c);
			continue;
		}

		c = fmt[++i] & 0xff;
		if (c == 0)
			break;
		switch (c) {
		case 'c':
			consputc(*argp++);
			break;
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if ((s = (char*)*argp++) == 0)
				s = "(null)";
			for (; *s; s++)
				consputc(*s);
			break;
		case '%':
			consputc('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
			consputc(c);
			break;
		}
	}

	if (locking)
		release(&cons.lock);

}

void
panic(char *s)
{
	cprintf("%s\n", s);
	for (;;) ;
}

static void
cgaputc(int c)
{
	int pos, tabstop;

	// Cursor position: col + 80*row.
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);

	if (c == '\n')
		pos += 80 - pos%80;
	else if (c == BACKSPACE) {
		if (pos >0) --pos;	
	} else if (c == '\t') {
		tabstop = (pos/8+1)*8;
		while (pos < tabstop)
			crt[pos++] =  ' ' | CRTATTR;
	} else
		crt[pos++] = (c&0xff) | CRTATTR;	// black on white

	if (pos < 0 || pos > 25 * 80)
		panic("pos under/overflow");

	if ((pos/80) >= 24) {
		memmove(crt, crt + 80, sizeof(crt[0])*23*80);		
		pos -= 80;
		memset(crt+pos, 0, sizeof(crt[0])*(24*80-pos));
	}

	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos >>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
	crt[pos] =  ' ' | CRTATTR;
}

static void
consputc(int c)
{
	cgaputc(c);
}


#define INPUT_BUF	128
struct {
	char buf[INPUT_BUF];
	uint r;	// read index
	uint w;	// write index
	uint e;	// edit index
} input;


void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;

	acquire(&cons.lock);
	while ((c = getc()) >= 0) {
		switch(c) {
		case C('P'):		// Process listing.
			// prodump() locks cons.lock indirectly; invoke later
			doprocdump = 1;
			break;
		case C('U'):   		// Kill line.
			while(input.e != input.w &&
					input.buf[(input.e-1) % INPUT_BUF] != '\n') {
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('H'):
		case '\x7f':
			if (input.e != input.w) {
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		default:
			if (c != 0 && input.e-input.r < INPUT_BUF) {
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				consputc(c);
				if (c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF) {
					input.w = input.e;
					wakeup(&input.r);
				}
			}
		}

	}
	release(&cons.lock);
	if (doprocdump) {
		//procdump();		// now call procdump() wo. cons.lock held
	}
}


int
consolewrite(struct inode *ip, char *buf, int n)
{
	int i;

	iunlock(ip);
	acquire(&cons.lock);
	for (i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);
	ilock(ip);
	return n;
}
int
consoleread(struct inode *ip, char *dst, int n)
{
	uint target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while (n > 0) {
		while (input.r == input.w) {
			if (current_proc->killed) {
				release(&cons.lock);
				ilock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}

		c = input.buf[input.r++ % INPUT_BUF];
		if (c == C('D')) {
			if (n < target) {
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result;
				//input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if (c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);
	return target - n;
}

void
consoleinit() {
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	picenable(IRQ_KBD);
	//ioapicenable(IRQ_KBD, 0);
}
