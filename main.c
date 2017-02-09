#include <types.h>
#include <x86.h>
#include <mmu.h>
#include <memlayout.h>
#include <defs.h>
#include <buf.h>

extern char end[];  // first address after kenel loaded from ELF file

void idestart(struct buf*);
extern struct  buf *idequeue;

// Bootstrap processor starts runnning C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
void main() {

	int i;
//	cprintf("hello %s%d [%x]!\n", "zhangyun", 596, 596);

//	cprintf("%s\n", kalloc());
	kinit1(end, P2V(4*1024*1024));  // phys page allocator
	kvmalloc();						// kernel page table
	mpinit();
	lapicinit();
	ioapicinit();
	seginit();						// segment descriptors

	consoleinit();

	extern char data[];
	cprintf("data: %x\n", data);
	cprintf("end: %x\n", end);
	picinit();						// interrupt controller
	/*
	sti();
	pushcli();
	pushcli();

	popcli();
	popcli();
	cli();
while(1) ;
*/

//	cprintf("%x\n", kalloc());
	pinit();						// process table

	tvinit();						// trap vectors
	idtinit();

	ideinit();						// ide
	/*
	struct buf b;
	b.dev = 0;
	b.blockno = 0;
	extern struct buf *idequeue;
	idequeue = &b;
	idestart(&b);
*/	
	binit();						// block cache
//	iinit();

	if (!ismp)
		timerinit();					// timer

//	userinit();

	kinit2(P2V(4*1024*1024), P2V(PHYSTOP));	// must come after startothers()
	userinit();
//	cprintf("created first process\n");
//	while(1);
//	cprintf("%s\n", "forever...");
//	while (1) ;
//asm volatile ("int $0");
//cprintf("return");
//while(1);
	
cprintf("into scheduler...\n");
	scheduler();
}
// The boot page table used in entry.S and entryother.S
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4M byte pages.
__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
	// Map VA's [0, 4MB) to PA's [0, 4MB)
	[0] = 0 | PTE_P | PTE_W | PTE_PS,
	// Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
	[KERNBASE >> PDXSHIFT] = 0 | PTE_P | PTE_W | PTE_PS,
};
