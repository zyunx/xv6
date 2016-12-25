// Mutual exclution spin locks

#include <types.h>
#include <defs.h>
#include <param.h>
#include <x86.h>
#include <spinlock.h>
#include <mmu.h>
#include <memlayout.h>

void
initlock(struct spinlock *lk, char *name)
{
	lk->name = name;
	lk->locked = 0;
}

// Check whether this cpu is holding the lock
int
holding(struct spinlock *lk)
{
	return lk->locked;
}


// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli. Also, if interrtups
// are off, then pushcli, popcli leaves them off
int ppcli_intena;
int ppcli_ncli = 0;
void
pushcli(void)
{
	int eflags;

	eflags = readeflags();
	cli();
	if (ppcli_ncli == 0)
		ppcli_intena = eflags & FL_IF;
	ppcli_ncli += 1;

	//cprintf("pushcli: ppcli_nlci %d ppcli_intena %d\n", ppcli_ncli, ppcli_intena);
}
void
popcli(void)
{
	if (readeflags() & FL_IF)
		panic("popcli - interruptible");
	if (--ppcli_ncli < 0)
		panic("popcli");
	if (ppcli_ncli == 0 && ppcli_intena)
		sti();

	//cprintf("popcli: ppcli_nlci %d ppcli_intena %d\n", ppcli_ncli, ppcli_intena);
}


// Record the current call stack in pcs[] by following the %ebp chain.
/*
void
getcallerpcs(void *v, uint pcs[])
{
	uint *ebp;
	int i;

	ebp = (uint*)v - 2;
	for (i = 0; i < 10; i++) {
		if (ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
			break;
		pcs[i] = ebp[1];		// saved %eip
		ebp = (uint*)ebp[0];	// saved %ebp
	}
	for (; i < 10; i++)
		pcs[i] = 0;
}
*/

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it
void
acquire(struct spinlock *lk)
{
	pushcli();			// disable interrtups to avoid deadlock.
	if (holding(lk))
		panic("acquire");

	// The xchg is atomic
	while(xchg(&lk->locked, 1) != 0)
		;

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that the critical sections's memory
	// references happen after the lock acquired
	__sync_synchronize();

	// Record info about lock acquisition for debugging.
//	getcallerpcs(&lk, lk->pcs);

}


// Release the lock
void
release(struct spinlock *lk)
{
	if (!holding(lk))
		panic("release");

	lk->pcs[0] = 0;

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that all stores in the critical
	// section are visible to other cores before the lock is released.
	// Both the C compiler and the hardware may re-order loads and stores;
	// __sync_synchronize() tells them both not to.
	__sync_synchronize();

	// Release the lock, equivalent to lk->locked = 0.
	// This code can't use a C assignment, since it might
	// not be atomic, A real OS would use C atomics here.
	asm volatile ("movl $0, %0" : "+m" (lk->locked) : );

	popcli();
}


