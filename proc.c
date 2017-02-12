/* proc.c */
#include <param.h>
#include <defs.h>
#include <proc.h>
#include <mmu.h>
#include <spinlock.h>

struct {
	struct spinlock lock; struct proc proc[NPROC];

} ptable;

struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
}
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
	struct proc *p;
	char *sp;

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == UNUSED)
			goto found;

	return 0;

found:
	p->state = EMBRYO;
	p->pid = nextpid++;

	// Allocate kernel stack.
	if ((p->kstack = kalloc()) == 0) {
		p->state = UNUSED;
		panic("out of memory");
		return 0;
	}
	sp = p->kstack + KSTACKSIZE;

	// Leave room for trap frame
	sp -= sizeof(*p->tf);
	p->tf = (struct trapframe*)sp;

	// Setup new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint*)sp = (uint)trapret;

	sp -= sizeof *p->context;
	p->context = (struct context*)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->eip = (uint) forkret;

	return p;
}

// A fork child's very first scheduling by scheduler()
// will swtch here. "Return" to use space
void forkret(void)
{
	static int first = 1;

	DBG_P("[forkret]\n");
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	//
	if (first) {
		// Some initilization functions must be run in the context
		// of a regular process (e.g., the call sleep), and thus cannot 
		// be run from main().
		first = 0;
		iinit(ROOTDEV);
		initlog(ROOTDEV);
	}
	//cprintf("..\n");
	//while(1);
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
	int i, pid;
	struct proc *np;

	DBG_P("[fork]\n");
	// Allocate process.
	if ((np = allocproc()) == 0) {
		return -1;
	}
	DBG_P("[fork] pid %d\n", np->pid);

	// Copy process state from p
	if ((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0) {
		cprintf("fork: copyuvm failed\n");
		kfree(np->kstack);
		np->kstack = 0;
		np->state = UNUSED;
		return -1;
	}
	np->sz = proc->sz;
	np->parent = proc;
	*np->tf = *proc->tf;

	// Clear %eax so that fork return 0 in the child
	np->tf->eax = 0;

	for (i = 0; i < NOFILE; i++)
		if (proc->ofile[i])
			np->ofile[i] = filedup(proc->ofile[i]);
	np->cwd = idup(proc->cwd);

	safestrcpy(np->name, proc->name, sizeof(proc->name));

	pid = np->pid;

	acquire(&ptable.lock);
	
	np->state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

// Exit the current process. Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
	struct proc *p;
	int fd;

	DBG_P("[exit]\n");
	if (proc == initproc)
		panic("init exiting");

	// Close all open files
	for (fd = 0; fd < NOFILE; fd++) {
		if (proc->ofile[fd]) {
			fileclose(proc->ofile[fd]);
			proc->ofile[fd] = 0;
		}
	}

	begin_op();
	iput(proc->cwd);
	end_op();
	proc->cwd = 0;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait()
	wakeup1(proc->parent);

	// Pass abandoned children to init.
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->parent == proc) {
			p->parent = initproc;
			if (p->state == ZOMBIE)
				wakeup1(initproc);
		}
	}


	// Jump into the scheduler, never to return.
	proc->state = ZOMBIE;
	sched();
	panic("zombie exit");
}

// Wait for child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
	struct proc *p;
	int havekids, pid;

	acquire(&ptable.lock);
	for (;;) {
		// Scan through table looking for exited children.
		havekids = 0;
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->parent != proc)
				continue;
			havekids = 1;
			if (p->state == ZOMBIE) {
				// Found one
				pid = p->pid;
				kfree(p->kstack);
				p->kstack = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->state = UNUSED;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children
		if (!havekids || proc->killed) {
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit. (See wakeup1 call in proc_exit.)
		sleep(proc, &ptable.lock);
	}
}

// setup first user process
void
userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];


	p = allocproc();

	initproc = p;
	if ((p->pgdir = setupkvm()) == 0)
		panic("userinit: out of memory?");
	inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);

	p->sz = PGSIZE;
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
	p->tf->es = p->tf->ds;
	p->tf->ss = p->tf->ds;
	p->tf->eflags = FL_IF;
	p->tf->esp = PGSIZE;
	p->tf->eip = 0;			// beginning of initcode.S

	safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd = namei("/");

	// this assignment ot p->state lets other cores
	// run this process. the acquire forces the above
	// writes to be visible, and the lock is also needed
	// because the assignment might not be atomic.
	acquire(&ptable.lock);

	p->state = RUNNABLE;

	release(&ptable.lock);
}



//static struct context *scheduler_context;
//struct proc *current_proc;
// process schedulre.
// Scheduler never returns. It loops, doing:
//  - choose a process to run
//  - swtch to start runnning that process
//  - eventually that process transfters control
//        via swtch  back to the scheduler
void
scheduler(void)
{
	struct proc *p;
//	static int count = 0;

	for (;;) {

		sti();
	//	cprintf("scheduler sti %d\n", ++count);
//		cprintf("scheduler eflags %d\n", readeflags()&FL_IF);
//		while (count > 10);
		// Loop over process table lokking for process to run
		acquire(&ptable.lock);

		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
//			cprintf("scheduler try proc %d\n", p - ptable.proc);
			if (p->state != RUNNABLE)
				continue;

			// Switch to chosen process. It is the process's job
			// to release ptable.lock and then reacquire it
			// before jumping back to use.
			proc = p;
			switchuvm(p);

			p->state = RUNNING;
			swtch(&cpu->scheduler, p->context);

			//cprintf("scheduler eflags IF %d\n", readeflags() & FL_IF);
			switchkvm();

			// Process is done running for now.
			// It should have changed its p->state before comming back.
			proc = 0;
		}
		release(&ptable.lock);
//		cprintf("scheduler release cli %d\n", readeflags());
	}

}


// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{

	if (proc == 0)
		panic("sleep");

	if (lk == 0)
		panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be 
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okey to release lk.
	if (lk != &ptable.lock) {		//DOC: sleeplock0
	//cprintf("sleep: acquire cli %d\n", readeflags() & FL_IF);
		acquire(&ptable.lock);		//DOC: sleeplock1
		release(lk);

	//cprintf("sleep: release cli %d\n", readeflags() & FL_IF);
	}

	// Go to sleep
	proc->chan = chan;
	proc->state = SLEEPING;

	sched();
/*
	cprintf("ok\n");
	while(1);
	*/
	// Tidy up.
	proc->chan = 0;

	// Reacquire original lock.
	if (lk != &ptable.lock) {

		release(&ptable.lock);
	//cprintf("sleep: release cli %d\n", readeflags() & FL_IF);
	//cprintf("sleep: acquire cli %d\n", readeflags() & FL_IF);
		acquire(lk);
	}

}

// Wake up all process sleeping on chan
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	struct proc *p;

//	cprintf("wakup1\n");
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == SLEEPING && p->chan == chan)
			p->state = RUNNABLE;
	}
}

// Wake up all processes sleeping on chan
void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// Enter scheduler. Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property o this kernel thread,
// not this CPU. It should be proc->intena and proc->ncli, 
// but that wolud break in the few places where a lock is held
// but there's no process
void
sched(void)
{
	int intena;

	if (!holding(&ptable.lock))
		panic("sched ptable.lock");
//	cprintf("ppcli_ncli: %x\n", ppcli_ncli);
	if (cpu->ncli != 1)
		panic("sched locks");
	if (proc->state == RUNNING)
		panic("sched running");
	if (readeflags() & FL_IF)
		panic("sched interruptible");

	intena = cpu->intena;
	//cprintf("sched: %d %d\n", intena, ppcli_ncli);
	swtch(&proc->context, cpu->scheduler);
	cpu->intena = intena;

	//cprintf("sched: cli %d\n", readeflags() & FL_IF);
	//if (proc->state);
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	acquire(&ptable.lock);
	proc->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}


// Grow current process's memory by n bytes
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
	uint sz;

	sz = proc->sz;
	if (n > 0) {
		if ((sz = allocuvm(proc->pgdir, sz, sz+n)) == 0)
			return -1;
	} else if (n < 0) {
		if ((sz = deallocuvm(proc->pgdir, sz, sz+n)) == 0)
			return -1;
	}
	proc->sz = sz;
	switchuvm(proc);
	return 0;
}
