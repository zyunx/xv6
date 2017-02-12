// Mutual exclusion lock.
#ifndef _SPINLOCK_H
#define _SPINLOCK_H

struct spinlock {
	uint locked;			// Is the lock held?

	// For debugging:
	char *name;				// Name of lock
	struct cpu *cpu;
	uint pcs[10];			// the call stack (an array of program counters)
							// that locked the lock
};

#endif
