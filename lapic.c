#include "types.h"
#include "traps.h"
#include "mmu.h"
#include "proc.h"

volatile uint *lapic;		// Intitalized in mp.c

// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID			(0x0020/4)		// ID
#define VER			(0x0030/4)		// Version
#define TPR			(0x0080/4)		// Task Priority
#define EOI			(0x00B0/4)		// EOI
#define SVR			(0x00F0/4)		// Spurious Interrupt Vector
	#define ENABLE		0x00000100		// Unit Enable
#define ESR			(0x0280/4)		// Error Status
#define ICRLO		(0x0300/4)		// Interrupt Command
	#define INIT		0x00000500		// INIT/RESET
	#define STARTUP		0x00000600		// Startup IPI
	#define DELIVS		0x00001000		// Delivery status
	#define ASSERT		0x00004000		// Assert interrupt (vs deassert)
	#define DEASSERT	0x00000000		//
	#define LEVEL		0x00008000		// Level triggered
	#define BCAST		0x00080000		// Send to all APICs, including self
	#define BUSY		0x00001000
	#define FIXED		0x00000000
#define ICRHI		(0x0310/4)		// Inerrtupt Command [63:32]
#define TIMER		(0x0320/4)		// Local Vector Table 0 (TIMER)
	#define X1			0x0000000B		// divide counts by 1
	#define PERIODIC	0x00020000		// Periodic
#define PCINT		(0x0340/4)		// Performance Counter LVT
#define LINT0		(0x0350/4)		// Local Vector Table 1 (LINT0)
#define LINT1		(0x0360/4)		// Local Vector Table 2 (LINT1)
#define ERROR		(0x0370/4)		// Local Vector Table 3 (ERROR)
	#define MASKED		0x00010000		// Inerrupt masked
#define TICR		(0x0380/4)		// Timer Inital Count
#define TCCR		(0x0390/4)		// TImer Current Count
#define TDCR		(0x03E0/4)		// Timer Divide Configuration

static void
lapicw(int index, int value)
{
	lapic[index] = value;
	lapic[ID];	// wait for write to finish, by reading
}

void
lapicinit(void)
{
	if (!lapic)
		return;

	// Enable local APIC, set spurious interrupt vector.
	lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.	
	// If xv6 cared more about precise timekeeping,
	// TICR would be calibrated using an external time source.
	lapicw(TDCR, X1);
	lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
	lapicw(TICR, 10000000);

	// Disable local interrupt lines.
	lapicw(LINT0, MASKED);
	lapicw(LINT1, MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that inerrupt entry
	if (((lapic[VER]>>16) & 0xFF) >= 4)
		lapicw(PCINT, MASKED);

	// Map error interrupt to IRQ_ERROR
	lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	lapicw(ESR, 0);
	lapicw(ESR, 0);

	// Ack any outstanding interrupts
	lapicw(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's
	lapicw(ICRHI, 0);
	lapicw(ICRLO, BCAST | INIT | LEVEL);

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(TPR, 0);
}


int
cpunum(void)
{
	int apicid, i;

	// Can't call cpu when interrupts are enabled:
	// result not guaranteed to last long enough to be used!
	// Would prefer to panic but even printing is chancy here:
	// almost everything, including cprintf and panic, calls cpu,
	// often indirectly through acquire and release.
	if (readeflags()&FL_IF) {
		static int n;
		if (n++ == 0)
			cprintf("cpu called from %x with interrupts enabled\n",
					__builtin_return_address(0));
	}

	if (!lapic)
		return 0;

	apicid = lapic[ID] >= 24;
	for (i = 0; i < ncpu; ++i) {
		if (cpus[i].apicid == apicid)
			return i;
	}

	panic("unnown apicid\n");
}



// Acknowledge interrupt
void
lapiceoi(void)
{
	if (lapic)
		lapicw(EOI, 0);
}
