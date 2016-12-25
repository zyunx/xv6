#ifndef _SLEEPLOCK_H
#define _SLEEPLOCK_H

#include <types.h>
#include <spinlock.h>

// Lock-term locks for processes
struct sleeplock {
	uint locked;			// Is the lock held?
	struct spinlock lk;		// spinlock protecting this sleep lock

	// For debugging
	char *name;				// name of lock
	int pid;				// Process holding lock
};

#endif
