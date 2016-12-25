#include <mmu.h>


.code16
.global start
start:
	cli			# disable interrupt

	xorw %ax, %ax
	movw %ax, %ds

# test a20
/*
	movw $0xffff, %ax
	movw %ax, %ds
	movw $0x7e0e, %bx
	movw (%bx), %dx

	cmpw $0x55AA, %dx

#	inb $0x92, %al
#	test %al, 2
	jz s

	movw $0xB000, %ax
	movw %ax, %ds
	movw $0x8000, %bx
	movb $'a', (%bx)
	movb $0x0f, 1(%bx)

s:
	jmp s	
*/

# Physical address line A20 is tied to zero so that the first PCs 
# with 2 MB would run software that assumed 1 MB.  Undo that.

seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60


#define	CR0_PE	0x00000001

	lgdt gdtdesc
	movl %cr0, %eax
	orl  $CR0_PE, %eax
	movl %eax, %cr0

#define SEG_KCODE	1
#define SEG_KDATA	2

	ljmp $(SEG_KCODE << 3), $start32

.code32
start32:
	movw $(SEG_KDATA << 3), %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0, %ax
	movw %ax, %fs
	movw %ax, %gs

	movl $0x7c00, %esp
	call bootmain

	/* step a 
	movl $0x100000, %ebx
	movb $'a', (%ebx)
	movb (%ebx), %al

	movl $0xB8000, %ebx
	movb %al, (%ebx)
	incl %ebx
	movb $0x0f, (%ebx)
*/
# forever loop
spin:
	jmp spin

#define SEG_NULL_ASM		\
	.word 0, 0 ;		\
	.byte 0, 0, 0, 0

#define SEG_ASM(type, base, limit)		\
	.word (((limit)>>12) & 0xffff) ;	\
	.word ((base) & 0xffff) ;		\
	.byte (((base) >> 16) & 0xff) ;		\
	.byte ((type) | 0x90)	;		\
	.byte (((limit)>>28) | 0xC0) ;		\
	.byte (((base) >> 24) & 0xff)

gdt:
	SEG_NULL_ASM
	SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
	SEG_ASM(STA_W, 0, 0xffffffff)

gdtdesc:
	.word (gdtdesc - gdt - 1)
	.long gdt

