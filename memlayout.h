// OS memory layout
#ifndef _MEMLAYOUT_H
#define _MEMLAYOUT_H

//#include <types.h>

#define EXTMEM		0x100000		// start of extened memory
#define PHYSTOP		0xE000000		// Top Physical menmory
#define DEVSPACE	0xFE000000		// Other devices are at high addresses

// Key address for address space layout (see kmap in vm.c for layout)
#define KERNBASE	0x80000000			// First kernel virtual address
#define KERNLINK	(KERNBASE+EXTMEM)	// Address where kernel is linked

#define V2P(a)	(((uint) (a)) - KERNBASE)
#define P2V(a)	(((void *) (a)) + KERNBASE)

#define V2P_WO(x)	((x) - KERNBASE)	// same as V2P, but without casts
#define P2V_WO(x)	((x) + KERNBASE)	// same as P2V, but without casts

#endif
