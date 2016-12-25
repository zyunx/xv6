#ifndef _BUF_H
#define _BUF_H

#include <types.h>
#include <fs.h>
#include <sleeplock.h>

struct buf {
	uint flags;
	uint dev;
	uint blockno;
	struct sleeplock lock;
	uint refcnt;
	struct buf *prev;		// LRU cache list
	struct buf *next;
	struct buf *qnext;		// disk queue
	uchar data[BSIZE];
};

#define B_VALID	0x2		// buffer has been read from disk
#define B_DIRTY	0x4		// buffer needs to be written to disk

#endif
