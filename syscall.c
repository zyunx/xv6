#include <defs.h>
#include <proc.h>
#include <syscall.h>

// User code makes a syscall with INT T_SYSCALL.
// System call number in %eax
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argumnet.

// Fetch the int at addr from the current process
int
fetchint(uint addr, int *ip)
{
//	cprintf("fetchint: addr=%x\n", addr);

	if (addr >= current_proc->sz || addr + 4 > current_proc->sz)
		return -1;
	*ip = *(int*)(addr);
	return 0;
}

// Fetch the null-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including null.
int
fetchstr(uint addr, char **pp)
{
	char *s, *ep;

	if (addr >= current_proc->sz)
		return -1;
	*pp = (char*)addr;
	ep = (char*)current_proc->sz;
	for (s = *pp; s < ep; s++)
		if (*s == 0)
			return s - *pp;
	return -1;
}

// Fetch the nth 32-bit system call argument as a pointer
// to a block of memory of size bytes. Check that the pointer
// lies within the process address space
int
argptr(int n, char **pp, int size)
{
	int i;

	if (argint(n, &i) < 0)
		return -1;
	if (size < 0 || (uint)i >= current_proc->sz || (uint)i+size > current_proc->sz)
		return -1;
	*pp =(char*)i;
	return 0;
}

int
argint(int n, int *ip)
{
	//cprintf("argint: addr=%x\n", current_proc->tf->esp+4+4*n);
	return fetchint(current_proc->tf->esp + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the point is valid and the string is null-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernl.)
int
argstr(int n, char **pp)
{
	int addr;
	if (argint(n, &addr) < 0)
		return -1;

	return fetchstr(addr, pp);
}

static int sys_console() {
	char *p;
	
	if (argstr(0, &p) < 0) {
		panic("memory access volation\n");
	}

	cprintf(p);
/*
	struct buf * f = bread(0, 0);
	cprintf("sector flag %x%x\n", f->data[510], f->data[511]);
	//cprintf("console pid %x\n", current_proc->pid);
	brelse(f);

	//cprintf("console pid %x\n", current_proc->pid);
	f = bread(0, 1001);
	uint *sb = (uint*) f->data;
	cprintf("superblock:\n");
	cprintf("size %x nblocks %x ninodes %x nlog %x\n logstart %x inodestart %x bmapstart %x\n", sb[0], sb[1], sb[2], sb[3], sb[4], sb[5], sb[6]);
//	cprintf("sector flag %x%x\n", f->data[510], f->data[511]);
	//cprintf("console pid %x\n", current_proc->pid);
	brelse(f);
	*/
	//extern void test_fs(void);
	//test_fs();
	//while(1);
	return 0;
};

extern int sys_getpid();
extern int sys_open();
extern int sys_dup();
extern int sys_close();
extern int sys_read();
extern int sys_write();
extern int sys_exec();
extern int sys_mknod();
extern int sys_mkdir();
extern int sys_fork();
extern int sys_exit();
extern int sys_wait();


static int (*syscalls[])(void) = {
	[SYS_getpid] = sys_getpid,
	[SYS_open] = sys_open,
	[SYS_dup] = sys_dup,
	[SYS_close] = sys_close,
	[SYS_console]	= sys_console,
	[SYS_read] = sys_read,
	[SYS_write] = sys_write,
	[SYS_exec] = sys_exec,
	[SYS_mknod] = sys_mknod,
	[SYS_mkdir] = sys_mkdir,
	[SYS_fork] = sys_fork,
	[SYS_exit] = sys_exit,
	[SYS_wait] = sys_wait,
};


void
syscall(void)
{
	int num;

	//cprintf("syscall user esp=%x\n", current_proc->tf->esp);
	num = current_proc->tf->eax;
	if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {

		current_proc->tf->eax = syscalls[num]();
	} else {
		cprintf("%d %s: unknown syscall %d\n",
				current_proc->pid, current_proc->name, num);
		current_proc->tf->eax = -1;
	}
}
