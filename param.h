#ifndef _PARAM_H
#define _PARAM_H

#define	NPROC			64				// maximum number of processes
#define KSTACKSIZE		4096			// size of per-process kernel stack
#define NOFILE			16				// oepn files per process
#define NFILE			100				// open files per system
#define NINODE			50				// maximum number of active i-nodes
#define NDEV			10				// maximum major device number
#define ROOTDEV			1				// device number of file system root disk
#define MAXARG			32				// max exec arguments
#define MAXOPBLOCKS		10				// max # of blocks any FS op writes
#define LOGSIZE			(MAXOPBLOCKS*3)	// max data blocks in on-disk log
#define NBUF			(MAXOPBLOCKS*3) // size of disk block cache
#define FSSIZE			10000			// size of file system in blocks


#define DEBUG		1

#ifdef DEBUG
#  define DBG_P(fmt, args...) cprintf(fmt, ##args)
#else
#  define DBG_P(fmt, args...)
#endif

#endif
