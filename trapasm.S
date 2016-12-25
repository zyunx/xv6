#include <mmu.h>

.global alltraps
alltraps:
	# Build trap frame
	pushl %ds
	pushl %es
	pushl %fs
	pushl %fs
	pushal

	# set up data and per-cpu segments.
	movw $(SEG_KDATA<<3), %ax
	movw %ax, %ds
  	movw %ax, %es
	
	# Call trap(tf), where tf=%esp
	pushl %esp
	call trap
	addl $4, %esp

	# Return falls through to trapret ...
.global trapret
trapret:
	popal
	popl %gs
	popl %fs
	popl %es
	popl %ds
	addl $0x8, %esp    # trapno and errcode

#	push $debug_msg
#	call cprintf
#f:
#	jmp f
#	movl $SEG_UDATA, %eax
#	movl %eax, %ds
#	movl %eax, %es
#	pushl %eax
#	movl $0xfff, %eax
#	pushl %eax
#	pushf
#	movl $SEG_U, %eax
#	pushl %eax
#	movl $0, %eax
#	pushl %eax
#	iret
#movl 0, %eax
	iret

debug_msg:
	.asciz "\ntrapret  %x %x %x %x %x\n"
