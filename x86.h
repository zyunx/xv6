#ifndef _X86_H
#define _X86_H

#include <types.h>

static inline uint
readeflags(void)
{
	uint eflags;
	asm volatile("pushfl; popl %0" : "=r" (eflags));
	return eflags;

}


static inline uint
xchg(volatile uint *addr, uint newval)
{
	uint result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile ("lock; xchgl %0, %1" :
			"+m" (*addr), "=a" (result) :
			"1" (newval) :
			"cc");
	return result;
}

static inline uint
rcr2(void)
{
	uint val;
	asm volatile("movl %%cr2, %0" : "=r" (val));
	return val;
}

static inline void
cli()
{
	asm volatile("cli");
}

static inline void
sti()
{
	asm volatile("sti");
}

static inline uchar
inb(ushort port)
{
	uchar data;
	
	asm volatile("inb %%dx, %%al"
			: "=a" (data)
			: "d" (port));

	return data;
}

static inline void
insl(int port, void *addr, int cnt)
{
	asm volatile("cld; rep insl"
			: "=D" (addr), "=c" (cnt)
			: "0" (addr), "d" (port), "1" (cnt)
			: "memory", "cc" );
}

static inline void
outb(ushort port, uchar data)
{
	asm volatile("outb %%al, %%dx"
			:
			: "d" (port), "a" (data));
}

static inline void
outw(ushort port, ushort data)
{
	asm volatile("outw %%ax, %%dx"
			:
			: "d" (port), "a" (data));
}

static inline void
outsl(int port, const void *addr, int cnt)
{
	asm volatile ("cld; rep outsl" 
			: "=S" (addr), "=c" (cnt)
			: "d" (port), "0"(addr), "1" (cnt)
			: "cc");
}

static inline void
stosb(void *addr, int data, int cnt)
{
	asm volatile ("cld; rep stosb"
			: "=D" (addr), "=c" (cnt)
			: "0" (addr), "1" (cnt), "a" (data)
			: "memory", "cc");
}

static inline void
stosl(void *addr, int data, int cnt)
{
	asm volatile ("cld; rep stosl"
			: "=D" (addr), "=c" (cnt)
			: "D" (addr), "c" (cnt), "a" (data)
			: "memory" "cc");
}


struct segdesc;

static inline void
lgdt(struct segdesc *p, int size)
{
	volatile ushort pd[3];

	pd[0] = size - 1;
	pd[1] = (uint)p;
	pd[2] = (uint)p >> 16;

	asm volatile ("lgdt (%0)" : : "r" (pd));
}


struct gatedesc;

static inline void
lidt(struct gatedesc *p, int size)
{
	volatile ushort pd[3];

	pd[0] = size - 1;
	pd[1] = (uint)p;
	pd[2] = (uint)p >> 16;

	asm volatile("lidt (%0)" : : "r" (pd));
}


static inline void
ltr(ushort sel)
{
	asm volatile("ltr %0" : : "r" (sel));
}


static inline void
lcr3(uint val)
{
	asm volatile("movl %0, %%cr3" : : "r" (val));
}

// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
	// registers as pushed by pusha
	uint edi;
	uint esi;
	uint ebp;
	uint oesp;
	uint ebx;
	uint edx;
	uint ecx;
	uint eax;

	// rest of trap frame
	ushort gs;
	ushort padding1;
	ushort fs;
	ushort padding2;
	ushort es;
	ushort padding3;
	ushort ds;
	ushort padding4;
	uint trapno;

	// below here defined by x86 hardware
	uint err;
	uint eip;
	ushort cs;
	ushort padding5;
	uint eflags;

	// below here only when crossing rings, such as from user to kernel
	uint esp;
	ushort ss;
	ushort padding6;
};

static inline void
loadgs(ushort v)
{
	asm volatile("movw %0, %%gs" : : "r" (v));
}

#endif
