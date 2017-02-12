#include <proc.h>
#include <defs.h>

int
sys_getpid(void)
{
//	cprintf("getpid %x\n", current_proc->pid);
	return proc->pid;
}

int
sys_fork(void)
{
	return fork();
}

int
sys_exit(void)
{
	exit();
	return 0;		// not reached
}

int
sys_wait(void)
{
	return wait();
}

int
sys_sbrk(void)
{
	int addr;
	int n;

	if (argint(0, &n) < 0)
		return -1;
	addr = proc->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}
