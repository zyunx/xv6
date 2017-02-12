#include <param.h>
#include <defs.h>
#include <types.h>
#include <traps.h>
#include <x86.h>
#include <mmu.h>
#include <proc.h>
#include <syscall.h>

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];		// in vectors.S: array of 256 entry pointers
uint ticks;

void
tvinit(void)
{
	int i;
	for (i = 0; i < 256; i++)
		SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);

	SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

	//initlock(&tickslock, "time");
}

void
idtinit(void)
{
	lidt(idt, sizeof(idt));
}


void
trap(struct trapframe *tf)
{
	uint cr2, cr3;

	//cprintf("trap %x\n", tf->trapno);
	if (tf->trapno == T_SYSCALL) {
		proc->tf = tf;
		syscall();
		return;
	}

	switch(tf->trapno) {
		case T_IRQ0 + IRQ_TIMER:
			ticks++;
			//if (ticks%100 == 0)
			//	cprintf("timer: .\n");
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_IDE:
			//cprintf("ide intr\n");
			ideintr();
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_KBD:
			kbdintr();
			lapiceoi();
			break;
		case T_IRQ0 + 7:
		case T_IRQ0 + IRQ_SPURIOUS:
			cprintf("spurious interrupt at %x:%x\n",
					tf->cs, tf->eip);
			lapiceoi();
			break;
			/*
		case T_IRQ0 + IRQ_ERROR:
			lapiceoi();
			break;
			*/
		default:
			if (proc ==0 || (tf->cs & 3) == 0) {
				// In kernel, it must be our mistake.
				cprintf("unexpected trap %d from eip %x (cr2=0x%x)\n",
						tf->trapno, tf->eip, rcr2());
				panic("trap");
			}

			// In user space, assume process misbehaved.
			cprintf("pid %d %s: trap %d err %d "
					"eip 0x%x addr 0x%x -- kill proc\n",
					proc->pid, proc->name, tf->trapno,
					tf->err, tf->eip, rcr2());
			proc->killed = 1;
			panic("error trap");
	}


//	cprintf("trap end%x\n", tf->trapno);
	// Force process to give up CPU on clock tick.
	// If interrupts were on while locks held, would need to check nlock.
//	cprintf("proc %x\n", proc);

	if (proc && proc->state == RUNNING 
			&& tf->trapno == T_IRQ0+IRQ_TIMER) {
		//cprintf("cpu %d yield\n", cpunum());
		yield();
	}
/*
if (tf->trapno != 32) {
	volatile int i = 0, j = 0;
	while (i < 400000000) {
		i++;
		//j = 0;
	}
	//while(1);
	}
	*/
//	cprintf("...\n");
//	i = 0;
//	while (i++ < 400000000);
}
