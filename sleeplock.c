#include <sleeplock.h>
#include <spinlock.h>
#include <proc.h>
#include <types.h>

void
initsleeplock(struct sleeplock *lk, char *name)
{
	initlock(&lk->lk, "sleep lock");
	lk->name = name;
	lk->locked = 0;
	lk->pid = 0;
}


void
acquiresleep(struct sleeplock *lk)
{
	acquire(&lk->lk);
	while(lk->locked) {
		sleep(lk, &lk->lk);
	}
	lk->locked = 1;
	lk->pid = proc->pid;
//	cprintf("sleep pid %x\n", lk->pid);
	release(&lk->lk);
}


void
releasesleep(struct sleeplock *lk)
{
	acquire(&lk->lk);
	lk->locked = 0;
	lk->pid = 0;
//	cprintf("bw");
	wakeup(lk);
//	cprintf("aw");
	release(&lk->lk);
}


int
holdingsleep(struct sleeplock *lk)
{
	int r;

	acquire(&lk->lk);
	r = lk->locked;
	release(&lk->lk);
	return r;
}
