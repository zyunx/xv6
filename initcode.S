// Initial process execs /init.
// This code runs in user space.

#include <syscall.h>
#include "traps.h"
#include <fcntl.h>

.globl start

start:
	pushl $initmsg	
	movl $SYS_console, %eax
	pushl $0
	int $0x40
	addl $8, %esp

// exec
	pushl $argv
	pushl $init
	pushl $0
	movl $SYS_exec, %eax
	int $T_SYSCALL

// read file content
/*	
	pushl $O_RDONLY
	pushl $fpath
	movl $SYS_open, %eax
	pushl $0
	int $0x40
	addl $12, %esp

	// save fd
	movl %eax, fdesc

	cmpl $0, %eax
	jl  lno

lok:
	pushl $ok
	movl $SYS_console, %eax
	pushl $0
	int $0x40
	addl $8, %esp


	pushl $256
	pushl $filecontent
	movl fdesc, %edx
	pushl %edx
	pushl $0
	movl $SYS_read, %eax
	int $0x40
	addl $16, %esp

	jmp close
lno:
	pushl $no	
	movl $SYS_console, %eax
	pushl $0
	int $0x40
	addl $8, %esp

close:
	movl fdesc, %eax
	pushl %eax
	movl $SYS_close, %eax
	pushl $0
	int $0x40
	addl $12, %esp
*/
exit:
	movl $SYS_exit, %eax
	int $T_SYSCALL
	jmp exit


forever:
	jmp forever
	//.byte 0xeb, 0xfe

ok:
	.asciz "ok\n"
no:
	.asciz "no\n"

initmsg:
	.asciz "hello, welcome to userspace\n"

fpath:
	.asciz "/hello.txt"


.comm fdesc, 4

.comm  filecontent, 256

init:
	.asciz "init"
.p2align 2
argv:
	.long init
	.long 0


