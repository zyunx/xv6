#include <mmu.h>
#include <memlayout.h>
#include <param.h>

.p2align 2
.text

.global _start
_start =  V2P_WO(entry)

entry:
	// Turn on page size extension for 4Mbyte pages
	movl	%cr4, %eax
	orl	$(CR4_PSE), %eax
	movl	%eax, %cr4
	// Set page directory
	movl	$(V2P_WO(entrypgdir)), %eax
	movl	%eax, %cr3
	// Turn on paging
	movl	%cr0, %eax
	orl		$(CR0_PG), %eax
	movl	%eax, %cr0
	// Set up the stack pointer
	movl	$(stack + KSTACKSIZE), %esp

	mov $main, %eax
	jmp	*%eax

.comm stack, KSTACKSIZE

