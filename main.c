#include <types.h>
#include <x86.h>
#include <mmu.h>
#include <memlayout.h>
#include <defs.h>
#include <buf.h>
#include "proc.h"

extern char end[];  // first address after kenel loaded from ELF file

static void startothers(void);
static void mpmain(void)	__attribute__((noreturn));
extern pde_t *kpgdir;

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

	seginit();						// segment descriptors

	picinit();						// interrupt controller
	
	ioapicinit();

	consoleinit();
/*
	extern char data[];
	cprintf("data: %x\n", data);
	cprintf("end: %x\n", end);
*/
	pinit();						// process table

	tvinit();						// trap vectors

	binit();						// block cache

	fileinit();

	ideinit();						// ide

	if (!ismp)
		timerinit();					// timer
	/*
	struct buf b;
	b.dev = 0;
	b.blockno = 0;
	extern struct buf *idequeue;
	idequeue = &b;
	idestart(&b);
*/	
	startothers();

	kinit2(P2V(4*1024*1024), P2V(PHYSTOP));	// must come after startothers()
	userinit();

//	cprintf("created first process\n");
//	while(1);
//	cprintf("%s\n", "forever...");
//	while (1) ;
//asm volatile ("int $0");
//cprintf("return");
//while(1);
	mpmain();	
}

// Other CPUs jump here from entryother.S
static void
mpenter(void)
{
	switchkvm();
	seginit();
	lapicinit();
	mpmain();
}

// Common CPU setup code
static void
mpmain(void)
{
	cprintf("cpu%d: starting\n", cpunum());
	idtinit();		// load idt register
	xchg(&cpu->started, 1);
	scheduler();
}

pde_t entrypgdir[];			// For entry.S

// Start the non-boot (AP) processors
static void
startothers(void)
{
	extern uchar _binary_entryother_start[], _binary_entryother_size[];
	uchar *code;
	struct cpu *c;
	char *stack;

	// Write entry code to unused memory at 0x7000.
	// The liner has placed the image of entryother.S in
	// _bianry_entryother_start.
	code = P2V(0x7000);
	memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == cpus+cpunum())  // We've started already
			continue;

		// Tell entryother.S what stack to use, where to enter, and what
		// pgdir to use. We cannot use kpgdir yet, because the AP processor
		// is running in low memory, so we use entrypgdir for the APs too.
		stack = kalloc();
		*(void**)(code-4) = stack + KSTACKSIZE;
		*(void**)(code-8) = mpenter;
		*(int**)(code-12) = (void*) V2P(entrypgdir);

		lapicstartap(c->apicid, V2P(code));
		// wait for cpu to finish mpmain()
		while(c->started == 0)
			;
	}
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
