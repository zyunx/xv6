#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
	.global name;\
	name:\
		movl $SYS_ ## name, %eax;\
		int $T_SYSCALL;	\
		ret

SYSCALL(exec)
SYSCALL(console)
SYSCALL(mknod)
SYSCALL(mkdir)
SYSCALL(open)
SYSCALL(close)
SYSCALL(dup)
SYSCALL(write)
SYSCALL(read)
SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(sbrk)
