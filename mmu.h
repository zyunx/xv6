#ifndef _MMU_H
#define _MMU_H

#include <types.h>

// Eflags register 
#define FL_CF				0x00000001		// Carry flag
#define FL_IF				0x00000200		// Interrupt flag

// Control Register flags
#define CR0_PE	0x00000001	// Protection Enable
#define CR0_PG  0x80000000  // Paging

#define CR4_PSE	0x00000010	// Page size extension

// segment type
#define STA_A	0x1
#define STA_R	0x2
#define STA_W	0x2
#define STA_X	0x8

// system segment type bits
#define STS_T32A	0x9		// Available 32-bit TSS
#define STS_TG32	0xF		// 32-bit trap gate
#define STS_IG32	0xE		// 32-bit inerrtup gate


#define SEG_KCODE	1		// kernel code
#define SEG_KDATA	2		// kernel data+stack
#define SEG_KCPU	3		// kernl per-cpu data
#define SEG_UCODE	4		// user code
#define SEG_UDATA	5		// user data + stack
#define SEG_TSS		6		// this process's task state
// cpu->gdt[NSEGS] holds the above segments;
#define NSEGS		7

#ifndef __ASSEMBLER__
// Segment Descriptor
struct segdesc {
	uint lim_15_0 : 16;				// Low bits of segment limit
	uint base_15_0 : 16;			// Low bits of segment base address
	uint base_23_16 : 8;			// Middle bits of segment base address
	uint type : 4;					// Segment type (see STS_ constants)
	uint s : 1;						// 0 = system, 1 = application
	uint dpl : 2;					// Descriptor Privilege Level
	uint p : 1;						// Present
	uint lim_19_16 : 4;				// Hight bits of segment limit
	uint avl : 1;					// Unused (available for software use)
	uint rsv1 : 1;					// Reserved
	uint db : 1;					// 0 = 16-bit segment, 1 = 32-bit segment
	uint g : 1;						// Granularity: limit scaled by 4K when set
	uint base_32_24 : 8;			// Hight bits of segment base address
};

// Normal segment
#define SEG(type, base, lim, dpl) (struct segdesc)		\
{  	((lim) >> 12) & 0xffff, (uint)(base) & 0xffff,		\
	((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,		\
	((lim) >> 28), 0, 0, 1, 1, (uint)(base) >> 24 		\
}
#define SEG16(type, base, lim, dpl) (struct segdesc)	\
{	(lim) & 0xffff, (uint)(base) & 0xffff,				\
	((uint)(base) >> 16) & 0xff, type, 1, dpl, 1,		\
	(uint)(lim) >> 16, 0, 0, 1, 0, (uint)(base) >> 24	\
}
#endif

#define DPL_USER		0x3			// User DPL


// Page's
#define PGSIZE		4096	// bytes mapped by a page
#define NPDENTRIES	1024	// PDEs per page
#define NPTENTRIES	1024	// PTEs per page

#define PGSHIFT		12
#define PTXSHIFT	12
#define PDXSHIFT	22

// page directory index
#define PDX(va)		((((uint)(va)) >> PDXSHIFT) & 0x3FF)
// page table index
#define PTX(va)		((((uint)(va)) >> PTXSHIFT) & 0x3FF)

// page table/directory entry flags
#define PTE_P		0x001		// Present
#define PTE_W		0x002		// Writable
#define PTE_U		0x004		// User
#define PTE_PS		0x080		// 4M Page Size

#define PTE_ADDR(pte)	((uint)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)	((uint)(pte) & 0xFFF)

#define PGROUNDUP(addr)		(((addr) + PGSIZE - 1) & ~(PGSIZE-1))
#define PGROUNDDOWN(addr)	((addr) & ~(PGSIZE-1))


#ifndef __ASSEMBLER__

// Task state segment format
struct taskstate {
	uint link;					// Old ts selector
	uint esp0;					// Stack pointers and segment selectors
	ushort ss0;					//     after an increase in privilege level
	ushort padding1;
	uint esp1;
	ushort ss1;
	ushort padding2;
	uint esp2; 
	ushort ss2;
	ushort padding3;
	void *cr3;					// Page dierctory base
	uint *eip;					// Saved state from last task switch
	uint eflags;
	uint eax;
	uint ecx;
	uint edx;
	uint ebx;
	uint esp;
	uint ebp;
	uint esi;
	uint edi;
	ushort es;						// Even more saved state (segment selector)
	ushort padding4;
	ushort cs;
	ushort padding5;
	ushort ss;
	ushort padding6;
	ushort ds;
	ushort padding7;
	ushort fs;
	ushort padding8;
	ushort gs;
	ushort padding9;
	ushort ldt;
	ushort padding10;
	ushort t;						// Trap on task switch
	ushort iomb;					// I/O map base address
};


// gate descriptor for interrupts and traps
struct gatedesc {
	uint off_15_0 : 16;				// low 16 bits of offset in segment
	uint cs: 16;					// code segment selector
	uint args : 5;					// # args, 0 for interrupt/trap gates
	uint rsv1 : 3;					// reserved(should be zero I guess)
	uint type : 4;					// type (STS_{TG,IG32,TG32})
	uint s : 1;						// must be 0 (system)
	uint dpl : 2;					// descriptor (meaning new) privilege level
	uint p : 1;						// Present
	uint off_31_16 : 16;			// high bits of offset in segment
};

// Set up a normal interrupt/trap gate descriptor.
//  - istrap: 1 for a trap (=exception) gate, 0 for an interrupt gate.
//            interupt gate clears FL_IF, trap gate leaves FL_IF alone
//  - sel: Code segment selector for interrupt/trap handler
//  - off: offset in code segment for interrupt/trap handler
//  - dpl: Descriptor Privilege Level -
//			the privilege level required for software to invoke
//			this interrupt/trap gate explicityly using an int instruction
#define SETGATE(gate, istrap, sel, off, d)		\
{												\
	(gate).off_15_0 = (uint)(off) & 0xffff;		\
	(gate).cs = (sel);							\
	(gate).args = 0;							\
	(gate).rsv1 = 0;							\
	(gate).type = (istrap) ? STS_TG32 : STS_IG32;	\
	(gate).s = 0;								\
	(gate).dpl = (d);							\
	(gate).p = 1;								\
	(gate).off_31_16 = (uint)(off) >> 16;		\
}

#endif

#endif
