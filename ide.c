// Simple PIO-based (non-DMA) IDE driver code

#include <types.h>
#include <param.h>
#include <defs.h>
#include <spinlock.h>
#include <sleeplock.h>
#include <fs.h>
#include <buf.h>
#include <traps.h>
#include <x86.h>
#include <mmu.h>

#include <proc.h>

#define SECTOR_SIZE			512
#define IDE_BSY				0x80
#define IDE_DRDY			0x40
#define IDE_DF				0x20
#define IDE_ERR				0x01

#define IDE_CMD_READ		0x20
#define IDE_CMD_WRITE		0x30
#define IDE_CMD_RDMUL		0xc4
#define IDE_CMD_WRMUL		0xc5

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock whilte manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;

static int havedisk1;

// Wait for IDE disk to become ready
static int
idewait(int checkerr)
{
	int r;

	while (((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
		;
	if (checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
		return -1;

	return 0;
};

void
ideinit(void)
{
	int i;

	initlock(&idelock, "ide");
	picenable(IRQ_IDE);
	idewait(0);

	// Check if disk 1 is present
	outb(0x1f6, 0xe0 | (1<<4));
	for (i = 0; i < 1000; i++) {
		if (inb(0x1f7) != 0) {
			havedisk1 = 1;
			break;
		}
	}

	if (!havedisk1) {
		panic("disk1 not present!!!");
	}
	// Siwtch back to disk 0
	outb(0x1f6, 0xe0 | (0 << 4));
}


// Start the request for b. Caller must hold idelock.
void
idestart(struct buf *b)
{
	cprintf("idestart: begin %x flags %x dev %d blockno %d refcnt %d \n",
			b, b->flags, b->dev, b->blockno, b->refcnt);

	if (b == 0)
		panic("idestart");
	if (b->blockno >= FSSIZE)
		panic("incorrect blockno");

	int sector_per_block = BSIZE/SECTOR_SIZE;
	int sector = b->blockno * sector_per_block;
	int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ : IDE_CMD_RDMUL;
	int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

	if (sector_per_block > 7) panic("idestart");

	idewait(0);
	outb(0x3f6, 0);			// generate interrtup
	outb(0x1f2, sector_per_block);		// number of sectors
	outb(0x1f3, sector & 0xff);
	outb(0x1f4, (sector >> 8) & 0xff);
	outb(0x1f5, (sector >> 16) & 0xff);
	outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));

	if (b->flags & B_DIRTY) {
		outb(0x1f7, write_cmd);
		outsl(0x1f0, b->data, BSIZE/4);
//	b->flags |= B_VALID;
//	b->flags &= ~B_DIRTY;
//	cprintf("idestart: wwwwwwwake\n");
//	wakeup(b);

	} else {
		outb(0x1f7, read_cmd);
	}
}


// Interrupt handler
void
ideintr(void)
{
	struct buf *b;

	static int count = 0;

	// First queued buffer is the active request.
	
	acquire(&idelock);

	if ((b = idequeue) == 0) {
		release(&idelock);
		cprintf("spurious IDE interrupt");
		return;
	}
	idequeue = b->qnext;
	// Read data if needed
	if (!(b->flags & B_DIRTY) && idewait(1) >= 0)
		insl(0x1f0, b->data, BSIZE/4);

	// Wake process waiting for this buf.
	b->flags |= B_VALID;
	b->flags &= ~B_DIRTY;
//	cprintf("ideintr: %d wakeup %x\n", ++count, b);
	wakeup(b);

	//cprintf("idequeue: current %x\n", idequeue);
	// Start disk on next buf in queue.
	if (idequeue != 0) {
		idestart(idequeue);
	}

	release(&idelock);
	//cprintf("idelock %x\n", &idelock);
//	cprintf("idelock end\n");
}


// Sync buf with disk
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
	struct buf **pp;

	static int count = 0;


	if (!holdingsleep(&b->lock))
		panic("iderw: buf not locked");
	if ((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
		panic("iderw: nothing to do");
	
	//cprintf("iderw: acquire cli %d\n", readeflags() & FL_IF);
	acquire(&idelock);		// DOC: acquire-lock

	// Append b to idequeue
	b->qnext = 0;
	for (pp=&idequeue; *pp; pp=&(*pp)->qnext)
		;
	*pp = b;


	cprintf("iderw: queue %x next %x\n        %x dev %d blockno %d flags %x\n", idequeue, idequeue->qnext, b, b->dev, b->blockno, b->flags);
	// Start disk if neccessary
	if (idequeue == b)
		idestart(b);

	// Wait for request to finish
	while ((b->flags & (B_VALID|B_DIRTY)) != B_VALID) {
		//cprintf("pid %x\n", current_proc->pid);
		cprintf("iderw: %d tosleep %x flags %x\n", ++count, b, b->flags);
		sleep(b, &idelock);
	}

	release(&idelock);
	//cprintf("iderw: release cli %d\n", readeflags() & FL_IF);
//	cprintf("iderw: M\n");
}
