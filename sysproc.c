#include <proc.h>
#include <defs.h>

int
sys_getpid(void)
{
//	cprintf("getpid %x\n", current_proc->pid);
	return current_proc->pid;
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
